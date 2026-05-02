#include "LabAdapter.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace dstools {

Result<void> LabAdapter::importToLayers(const QString &filePath,
                                        std::map<QString, nlohmann::json> &layers,
                                        const ProcessorConfig & /*config*/) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return Result<void>::Error("Cannot open file: " + filePath.toStdString());

    const QString content = QTextStream(&file).readAll().trimmed();
    file.close();

    const QStringList syllables = content.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);

    nlohmann::json boundaries = nlohmann::json::array();
    int id = 1;
    for (const auto &syl : syllables) {
        boundaries.push_back({{"id", id}, {"pos", 0}, {"text", syl.toStdString()}});
        ++id;
    }
    // Final boundary with empty text
    boundaries.push_back({{"id", id}, {"pos", 0}, {"text", ""}});

    layers[QStringLiteral("grapheme")] = {{"name", "grapheme"}, {"boundaries", boundaries}};
    return Result<void>::Ok();
}

Result<void> LabAdapter::exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                          const QString &outputPath,
                                          const ProcessorConfig & /*config*/) {
    auto it = layers.find(QStringLiteral("grapheme"));
    if (it == layers.end())
        return Result<void>::Error("No grapheme layer found");

    const auto &boundaries = it->second["boundaries"];
    QStringList texts;
    for (const auto &b : boundaries) {
        const auto text = QString::fromStdString(b.value("text", ""));
        if (!text.isEmpty())
            texts.append(text);
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return Result<void>::Error("Cannot write file: " + outputPath.toStdString());

    QTextStream out(&file);
    out << texts.join(' ');
    file.close();
    return Result<void>::Ok();
}

} // namespace dstools
