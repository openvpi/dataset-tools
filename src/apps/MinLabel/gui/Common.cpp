#include "Common.h"
#include <QMessageBox>

QString audioFileToDsFile(const QString &filename) {
    QFileInfo info(filename);
    QString suffix = info.suffix().toLower();
    QString name = info.fileName();
    return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
           (suffix != "wav" ? "_" + suffix : "") + ".lab";
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

QString audioFileToTextFile(const QString &filename) {
    QFileInfo info(filename);
    QString suffix = info.suffix().toLower();
    QString name = info.fileName();
    return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
           (suffix != "wav" ? "_" + suffix : "") + ".txt";
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

bool copyFile(QList<CopyInfo> &copyList) {
    bool overwriteAll = false;
    bool skipAll = false;

    for (const CopyInfo &copyInfo : copyList) {
        if (copyInfo.exist && !overwriteAll && !skipAll) {
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(nullptr, "Overwrite?",
                                          QString("File %1 already exists, overwrite?").arg(copyInfo.filename),
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
            if (QFile::exists(copyInfo.targetAudio)) {
                QFile::remove(copyInfo.targetAudio);
            }

            if (QFile::exists(copyInfo.targetLab)) {
                QFile::remove(copyInfo.targetLab);
            }

            if (!QFile::copy(copyInfo.sourceAudio, copyInfo.targetAudio) ||
                !QFile::copy(copyInfo.sourceLab, copyInfo.targetLab)) {
                QMessageBox::critical(nullptr, "Copy failed",
                                      QString("Failed to copy file %1, exit").arg(copyInfo.filename));
                return false;
            }
        }
    }
    return true;
}

void mkdir(const QString &sourcePath, const QString &outputDir) {
    if (outputDir.isEmpty()) {
        QMessageBox::critical(nullptr, "Warning", "Output directory name is empty.");
        return;
    }

    if (QDir(outputDir).exists()) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Warning", "Output directory already exists, do you want to use it?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    if (!QDir(outputDir).exists() && !QDir(sourcePath).mkdir(outputDir)) {
        QMessageBox::critical(nullptr, "Warning", "Failed to create output directory.");
        return;
    }
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

                QString outputPath = output.absolutePath() + "/" + audioName;
                CopyInfo copyInfo(audioName, audioPath, outputPath, QFile(outputPath).exists());
                copyList.append(copyInfo);
            }
        }
    }
    return copyList;
}