#pragma once

#include <QString>
#include <QStringList>

#include <dstools/Result.h>

namespace Minlabel {

struct LabelData {
    QString lab;
    QString rawText;
    QString labWithoutTone;
    bool isCheck = false;
};

struct ConvertResult {
    int count = 0;
    QStringList failedFiles;
};

class MinLabelService {
public:
    static QString removeTone(const QString &labContent);

    static dstools::Result<LabelData> loadLabel(const QString &jsonFilePath);
    static dstools::Result<void> saveLabel(const QString &jsonFilePath, const QString &labFilePath,
                                  const LabelData &data);
    static dstools::Result<ConvertResult> convertLabToJson(const QString &dirName);
};

} // namespace Minlabel
