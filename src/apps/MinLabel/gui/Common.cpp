#include "Common.h"
#include <QDebug>
#include <QDir>
#include <QMessageBox>

#include <dstools/AudioFileResolver.h>
#include <dsfw/JsonHelper.h>

namespace Minlabel {

    QString audioToOtherSuffix(const QString &filename, const QString &tarSuffix) {
        return dstools::AudioFileResolver::audioToDataFile(filename, tarSuffix);
    }

    QString labFileToAudioFile(const QString &filename) {
        return dstools::AudioFileResolver::findAudioFile(filename);
    }

    bool expFile(const CopyInfo &copyInfo, const QString &item, const QString &suffix, const QString &data) {
        const QString target = copyInfo.targetDir + "/" + item + "/" + copyInfo.tarBasename + "." + suffix;
        if (QFile::exists(target)) {
            QFile::remove(target);
        }

        QFile tar(target);
    if (!tar.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "expFile: Failed to open file for writing:" << target << tar.errorString();
        return false;
    }
        QTextStream out(&tar);
        out << data;
        tar.close();
        return true;
    }

    int jsonCount(const QString &dirName) {
        const QDir directory(dirName);
        QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);

        int count = 0;
        for (const QFileInfo &fileInfo : fileInfoList) {
            if (fileInfo.suffix() == "json" && fileInfo.size() > 0) {
                std::string error;
                auto j = dstools::JsonHelper::loadFile(fileInfo.filePath().toStdString(), error);
                if (error.empty() && j.contains("isCheck") && j.value("isCheck", false)) {
                    count++;
                }
            }
        }
        return count;
    }

    bool copyFile(QList<CopyInfo> &copyList, const ExportInfo &exportInfo) {
        bool overwriteAll = false;
        bool skipAll = false;

        for (const CopyInfo &copyInfo : copyList) {
            if (copyInfo.exist && !overwriteAll && !skipAll) {
                const QMessageBox::StandardButton reply = QMessageBox::question(
                    nullptr, "Overwrite?", QString("File %1 already exists, overwrite?").arg(copyInfo.rawName),
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel | QMessageBox::YesToAll |
                        QMessageBox::NoToAll);
                if (reply == QMessageBox::Cancel) {
                    return false;
                } else if (reply == QMessageBox::No) {
                    continue;
                } else if (reply == QMessageBox::YesToAll) {
                    overwriteAll = true;
                } else if (reply == QMessageBox::NoToAll) {
                    skipAll = true;
                }
            }

            if (!(copyInfo.exist && skipAll)) {
                QString sourceAudio = copyInfo.sourceDir + "/" + copyInfo.rawName;
                QString sourceJson = audioToOtherSuffix(sourceAudio, "json");

                QString labContent, txtContent, unToneLab;
                nlohmann::json readData;
                if (readJsonFile(sourceJson, readData)) {
                    txtContent = QString::fromStdString(readData.value("raw_text", std::string{}));
                    labContent = QString::fromStdString(readData.value("lab", std::string{}));
                    unToneLab = QString::fromStdString(readData.value("lab_without_tone", std::string{}));
                }

                if (exportInfo.exportAudio) {
                    QString targetAudio = copyInfo.targetDir + "/wav/" + copyInfo.tarName;

                    if (QFile::exists(targetAudio)) {
                        QFile::remove(targetAudio);
                    }

                    if (!QFile::copy(sourceAudio, targetAudio)) {
                        QMessageBox::critical(nullptr, "Copy failed audio",
                                              QString("Failed to copy file %1, exit").arg(copyInfo.tarName));
                        return false;
                    }
                }

                if (exportInfo.labFile) {
                    if (!expFile(copyInfo, "lab", "lab", labContent)) {
                        QMessageBox::critical(
                            nullptr, "Copy failed lab",
                            QString("Failed to copy file %1.%2, exit").arg(copyInfo.tarBasename, "lab"));
                        return false;
                    }
                }

                if (exportInfo.rawText) {
                    if (!expFile(copyInfo, "raw_text", "txt", txtContent)) {
                        QMessageBox::critical(
                            nullptr, "Copy failed raw text",
                            QString("Failed to copy file %1.%2, exit").arg(copyInfo.tarBasename, "txt"));
                        return false;
                    }
                }

                if (exportInfo.removeTone) {
                    if (!expFile(copyInfo, "lab_without_tone", "lab", unToneLab)) {
                        QMessageBox::critical(
                            nullptr, "Copy failed lab without tone",
                            QString("Failed to make file %1.%2, exit").arg(copyInfo.tarBasename, "lab"));
                        return false;
                    }
                }
            }
        }
        return true;
    }

    void mkItem(const bool opt, const QString &folderPath, const QString &folderName) {
        if (opt) {
            const QString tarDir = folderPath + "/" + folderName;
            if (!QDir(tarDir).exists() && !QDir(folderPath).mkdir(folderName)) {
                QMessageBox::critical(nullptr, "Warning", "Failed to create " + folderName + " directory.");
            }
        }
    }

    void mkdir(const ExportInfo &exportInfo) {
        const QString folderPath = exportInfo.outputDir + "/" + exportInfo.folderName;
        if (exportInfo.outputDir.isEmpty()) {
            QMessageBox::critical(nullptr, "Warning", "Output directory name is empty.");
            return;
        }

        if (QDir(folderPath).exists()) {
            const QMessageBox::StandardButton reply =
                QMessageBox::question(nullptr, "Warning", "Output directory already exists, do you want to use it?",
                                      QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No) {
                return;
            }
        }

        if (!QDir(folderPath).exists() && !QDir(exportInfo.outputDir).mkdir(exportInfo.folderName)) {
            QMessageBox::critical(nullptr, "Warning", "Failed to create output directory.");
            return;
        }

        mkItem(exportInfo.exportAudio, folderPath, "wav");
        mkItem(exportInfo.labFile, folderPath, "lab");
        mkItem(exportInfo.rawText, folderPath, "raw_text");
        mkItem(exportInfo.removeTone, folderPath, "lab_without_tone");
    }

    QList<CopyInfo> mkCopylist(const QString &sourcePath, const QString &outputDir) {
        const QDir source(sourcePath);
        const QDir output(outputDir);
        QFileInfoList fileInfoList = source.entryInfoList(QDir::Files);

        QList<CopyInfo> copyList;
        for (const QFileInfo &fileInfo : fileInfoList) {
            if (fileInfo.suffix() == "json" && fileInfo.size() > 0) {
                const QString audioPath = labFileToAudioFile(fileInfo.absoluteFilePath());

                if (QFile(audioPath).exists()) {
                    auto audioInfo = QFileInfo(audioPath);
                    QString audioName = audioInfo.fileName();

                    QString targetPath = output.absolutePath() + "/wav/" + audioName;
                    CopyInfo copyInfo(audioName, audioName, source.absolutePath(), output.absolutePath(),
                                      QFile(targetPath).exists());
                    copyList.append(copyInfo);
                }
            }
        }
        return copyList;
    }

    bool readJsonFile(const QString &fileName, nlohmann::json &jsonData) {
        std::string error;
        jsonData = dstools::JsonHelper::loadFile(fileName.toStdString(), error);
        return error.empty();
    }

    bool writeJsonFile(const QString &fileName, const nlohmann::json &jsonData) {
        std::string error;
        return dstools::JsonHelper::saveFile(fileName.toStdString(), jsonData, error);
    }
}