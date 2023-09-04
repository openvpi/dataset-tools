#ifndef DATASET_TOOLS_COMMON_H
#define DATASET_TOOLS_COMMON_H

#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QString>
#include <QStringList>

#include "zhg2p.h"

QString audioFileToDsFile(const QString &filename);
QString labFileToAudioFile(const QString &filename);
QString audioFileToTextFile(const QString &filename);

struct CopyInfo {
    QString filename;
    QString sourceAudio;
    QString targetAudio;
    QString sourceLab;
    QString targetLab;
    bool exist;

    CopyInfo(QString fname, QString srcPath, QString outPath, bool isExist)
        : filename(std::move(fname)), sourceAudio(std::move(srcPath)), targetAudio(std::move(outPath)), exist(isExist) {
        sourceLab = audioFileToDsFile(sourceAudio);
        targetLab = audioFileToDsFile(targetAudio);
    }
};

bool copyFile(QList<CopyInfo> &copyList);
int labCount(const QString &dirName);
void mkdir(const QString &sourcePath, const QString &outputDir);
QList<CopyInfo> mkCopylist(const QString &sourcePath, const QString &outputDir, bool convertPinyin, IKg2p::ZhG2p *g2p_zh);

#endif // DATASET_TOOLS_COMMON_H
