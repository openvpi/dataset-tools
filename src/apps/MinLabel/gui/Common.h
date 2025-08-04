#ifndef DATASET_TOOLS_COMMON_H
#define DATASET_TOOLS_COMMON_H

#include <QFileInfo>
#include <QString>
#include <utility>

namespace Minlabel {
    QString audioToOtherSuffix(const QString &filename, const QString &tarSuffix);
    QString labFileToAudioFile(const QString &filename);

    struct ExportInfo {
        QString outputDir;
        QString folderName;
        bool exportAudio;
        bool labFile;
        bool rawText;
        bool removeTone;
    };

    struct CopyInfo {
        QString rawName;
        QString tarName;
        QString sourceDir;
        QString targetDir;
        QString tarBasename;
        bool exist;

        CopyInfo(QString rawName, const QString &tarName, QString sourceDir, QString targetDir, bool isExist)
            : rawName(std::move(rawName)), tarName(tarName), sourceDir(std::move(sourceDir)),
              targetDir(std::move(targetDir)), exist(isExist) {
            const QString filename = QFileInfo(tarName).fileName();
            const QString suffix = QFileInfo(tarName).suffix();
            tarBasename = filename.mid(0, filename.size() - suffix.size() - 1);
        }
    };

    bool copyFile(QList<CopyInfo> &copyList, const ExportInfo &exportInfo);
    int jsonCount(const QString &dirName);
    void mkdir(const ExportInfo &exportInfo);
    QList<CopyInfo> mkCopylist(const QString &sourcePath, const QString &outputDir);
    bool readJsonFile(const QString &fileName, QJsonObject &jsonObject);
    bool writeJsonFile(const QString &fileName, const QJsonObject &jsonObject);
}
#endif // DATASET_TOOLS_COMMON_H
