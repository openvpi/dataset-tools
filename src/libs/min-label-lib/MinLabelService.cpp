#include "MinLabelService.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

#include <dsfw/JsonHelper.h>
#include <dstools/AudioFileResolver.h>

#include <nlohmann/json.hpp>

namespace Minlabel {

QString MinLabelService::removeTone(const QString &labContent) {
    if (labContent.isEmpty() || !labContent.contains(" "))
        return {};
    static const QRegularExpression nonLowerAlpha("[^a-z]");
    QStringList inputList = labContent.split(" ");
    for (QString &item : inputList) {
        item.remove(nonLowerAlpha);
    }
    return inputList.join(" ");
}

dstools::Result<LabelData> MinLabelService::loadLabel(const QString &jsonFilePath) {
    auto jResult = dstools::JsonHelper::loadFile(jsonFilePath.toStdString());
    if (!jResult)
        return dstools::Result<LabelData>::Error(jResult.error());

    const auto &j = jResult.value();
    LabelData data;
    data.lab = QString::fromStdString(j.value("lab", std::string{}));
    data.rawText = QString::fromStdString(j.value("raw_text", std::string{}));
    data.labWithoutTone = QString::fromStdString(j.value("lab_without_tone", std::string{}));
    data.isCheck = j.value("isCheck", false);
    return dstools::Result<LabelData>::Ok(std::move(data));
}

dstools::Result<void> MinLabelService::saveLabel(const QString &jsonFilePath, const QString &labFilePath,
                                        const LabelData &data) {
    static const QRegularExpression multiSpace("\\s+");

    QString labContent = data.lab;
    QString withoutTone = data.labWithoutTone.isEmpty() ? removeTone(data.lab) : data.labWithoutTone;

    nlohmann::json writeData;
    writeData["lab"] = labContent.replace(multiSpace, " ").toStdString();
    writeData["raw_text"] = data.rawText.toStdString();
    writeData["lab_without_tone"] = withoutTone.replace(multiSpace, " ").toStdString();
    writeData["isCheck"] = true;

    auto jsonResult = dstools::JsonHelper::saveFile(jsonFilePath.toStdString(), writeData);
    if (!jsonResult)
        return dstools::Result<void>::Error(jsonResult.error());

    if (!labFilePath.isEmpty() && !labContent.isEmpty()) {
        QFile labFile(labFilePath);
        if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text))
            return dstools::Result<void>::Error("Failed to write lab file: " + labFilePath.toStdString());
        QTextStream labIn(&labFile);
        labIn << labContent;
        labFile.close();
    }

    return dstools::Result<void>::Ok();
}

dstools::Result<ConvertResult> MinLabelService::convertLabToJson(const QString &dirName) {
    const QDir directory(dirName);
    QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);

    ConvertResult result;
    for (const QFileInfo &fileInfo : fileInfoList) {
        QString suffix = fileInfo.suffix().toLower();
        if (suffix != "lab")
            continue;

        QString name = fileInfo.fileName();
        QString jsonFilePath =
            fileInfo.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) + ".json";

        if (QFile::exists(jsonFilePath))
            continue;

        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            result.failedFiles.append(fileInfo.absoluteFilePath());
            continue;
        }

        QString labContent = QString::fromUtf8(file.readAll());
        file.close();

        LabelData data;
        data.lab = labContent;
        data.rawText = labContent;
        data.labWithoutTone = removeTone(labContent);
        data.isCheck = true;

        auto saveResult = saveLabel(jsonFilePath, {}, data);
        if (!saveResult) {
            result.failedFiles.append(fileInfo.absoluteFilePath());
            continue;
        }

        result.count++;
    }

    return dstools::Result<ConvertResult>::Ok(std::move(result));
}

} // namespace Minlabel
