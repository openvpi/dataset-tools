#ifndef DATASET_TOOLS_COMMON_H
#define DATASET_TOOLS_COMMON_H

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <utility>

#include "zhg2p.h"

QString audioToOtherSuffix(const QString &filename, const QString &tarSuffix);
QString labFileToAudioFile(const QString &filename);

struct ExportInfo {
    QString outputDir;
    QString folderName;
    bool convertPinyin;
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
        tarBasename = tarName.split(".")[0];
    }
};

bool copyFile(QList<CopyInfo> &copyList, ExportInfo &exportInfo);
int jsonCount(const QString &dirName);
void mkdir(ExportInfo &exportInfo);
QList<CopyInfo> mkCopylist(const QString &sourcePath, const QString &outputDir, bool convertPinyin,
                           IKg2p::ZhG2p *g2p_zh);
bool readJsonFile(const QString &fileName, QJsonObject &jsonObject);
bool writeJsonFile(const QString &fileName, const QJsonObject &jsonObject);
#endif // DATASET_TOOLS_COMMON_H
