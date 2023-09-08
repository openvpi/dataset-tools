#include "Common.h"
#include <QDebug>
#include <QMessageBox>

QString audioToOtherSuffix(const QString &filename, const QString &tarSuffix) {
    QFileInfo info(filename);
    QString suffix = info.suffix().toLower();
    QString name = info.fileName();
    return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
           (suffix != "wav" ? "_" + suffix : "") + "." + tarSuffix;
}

QString labFileToAudioFile(const QString &filename) {
    QFileInfo info(filename);

    QString dirPath = info.absolutePath();
    QString baseName = info.baseName();

    QDir directory(dirPath);
    QStringList fileTypes;
    fileTypes << "*.wav"
              << "*.mp3"
              << "*.m4a"
              << "*.flac";

    QStringList audioFiles = directory.entryList(fileTypes, QDir::Files);
    for (const QString &audioFile : audioFiles) {
        if (QFileInfo(audioFile).baseName() == baseName) {
            QString audioFilePath = directory.absoluteFilePath(audioFile);
            return audioFilePath;
        }
    }
    return "";
}

bool coverCopy(const CopyInfo &copyInfo, const QString &item, const QString &suffix) {
    QString source = audioToOtherSuffix(copyInfo.sourceDir + "/" + copyInfo.rawName, suffix);
    QString target = copyInfo.targetDir + "/" + item + "/" + copyInfo.tarBasename + "." + suffix;
    if (QFile::exists(target)) {
        QFile::remove(target);
    }
    if (QFile::exists(source)) {
        return QFile::copy(source, target);
    } else {
        return true;
    }
}

int labCount(const QString &dirName) {
    QDir directory(dirName);
    QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);

    int count = 0;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        if (fileInfo.suffix() == "lab" && fileInfo.size() > 0) {
            count++;
        }
    }
    return count;
}

bool mkLabWithoutTone(const QString &labFile, const QString &tarFile) {
    if (QFile::exists(tarFile)) {
        QFile::remove(tarFile);
    }
    if (QFile::exists(labFile)) {
        QFile file(labFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        QTextStream in(&file);
        QString line = in.readLine();
        QStringList inputList = line.split(" ");
        for (QString &item : inputList) {
            item.remove(QRegExp("[^a-z]"));
        }
        QString result = inputList.join(" ");
        QFile tar(tarFile);
        if (!tar.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        QTextStream out(&tar);
        out << result;
        tar.close();
        file.close();
        return true;
    } else {
        return false;
    }
}

bool copyFile(QList<CopyInfo> &copyList, ExportInfo &exportInfo) {
    bool overwriteAll = false;
    bool skipAll = false;

    for (const CopyInfo &copyInfo : copyList) {
        if (copyInfo.exist && !overwriteAll && !skipAll) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, "Overwrite?",
                                          QString("File %1 already exists, overwrite?").arg(copyInfo.rawName),
                                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel |
                                              QMessageBox::YesToAll | QMessageBox::NoToAll);
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
            if (exportInfo.exportAudio) {
                QString sourceAudio = copyInfo.sourceDir + "/" + copyInfo.rawName;
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
                if (!coverCopy(copyInfo, "lab", "lab")) {
                    QMessageBox::critical(nullptr, "Copy failed lab",
                                          QString("Failed to copy file %1.%2, exit").arg(copyInfo.tarBasename, "lab"));
                    return false;
                }
            }

            if (exportInfo.rawText) {
                if (!coverCopy(copyInfo, "raw_text", "txt")) {
                    QMessageBox::critical(nullptr, "Copy failed raw text",
                                          QString("Failed to copy file %1.%2, exit").arg(copyInfo.tarBasename, "txt"));
                    return false;
                }
            }

            if (exportInfo.removeTone) {
                QString labFile = audioToOtherSuffix(copyInfo.sourceDir + "/" + copyInfo.rawName, "lab");
                QString tarFile = copyInfo.targetDir + "/lab_without_tone/" + copyInfo.tarBasename + ".lab";
                if (!mkLabWithoutTone(labFile, tarFile)) {
                    QMessageBox::critical(nullptr, "Copy failed lab without tone",
                                          QString("Failed to make file %1.%2, exit").arg(copyInfo.tarBasename, "lab"));
                    return false;
                }
            }
        }
    }
    return true;
}

void mkItem(bool opt, QString &folderPath, const QString &folderName) {
    if (opt) {
        QString tarDir = folderPath + "/" + folderName;
        if (!QDir(tarDir).exists() && !QDir(folderPath).mkdir(folderName)) {
            QMessageBox::critical(nullptr, "Warning", "Failed to create " + folderName + " directory.");
            return;
        }
    }
}

void mkdir(ExportInfo &exportInfo) {
    QString folderPath = exportInfo.outputDir + "/" + exportInfo.folderName;
    if (exportInfo.outputDir.isEmpty()) {
        QMessageBox::critical(nullptr, "Warning", "Output directory name is empty.");
        return;
    }

    if (QDir(folderPath).exists()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Warning", "Output directory already exists, do you want to use it?",
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

QList<CopyInfo> mkCopylist(const QString &sourcePath, const QString &outputDir, bool convertPinyin,
                           IKg2p::ZhG2p *g2p_zh) {
    QDir source(sourcePath);
    QDir output(outputDir);
    QFileInfoList fileInfoList = source.entryInfoList(QDir::Files);

    QList<CopyInfo> copyList;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        if (fileInfo.suffix() == "lab" && fileInfo.size() > 0) {
            QString audioPath = labFileToAudioFile(fileInfo.absoluteFilePath());

            if (QFile(audioPath).exists()) {
                QFileInfo audioInfo = QFileInfo(audioPath);
                QString audioName = audioInfo.fileName();

                if (convertPinyin) {
                    QString basename = audioInfo.baseName();
                    QRegExp rx("[\\x4e00-\\x9fa5]+");
                    int pos = 0;
                    while ((pos = rx.indexIn(basename, pos)) != -1) {
                        auto tPinyin = (g2p_zh->convert(rx.cap(0), false)).split(" ").join("");
                        basename.replace(pos, rx.matchedLength(), tPinyin);
                        pos += rx.matchedLength();
                    }
                    audioName = basename + "." + audioInfo.suffix();
                }

                QString targetPath = output.absolutePath() + "/wav/" + audioName;
                CopyInfo copyInfo(audioInfo.fileName(), audioName, source.absolutePath(), output.absolutePath(),
                                  QFile(targetPath).exists());
                copyList.append(copyInfo);
            }
        }
    }
    return copyList;
}