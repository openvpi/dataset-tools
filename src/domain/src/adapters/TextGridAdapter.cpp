#include "TextGridAdapter.h"

#include <dstools/TextGridToCsv.h>
#include <dstools/TimePos.h>

namespace dstools {

Result<void> TextGridAdapter::importToLayers(const QString &filePath,
                                             std::map<QString, nlohmann::json> &layers,
                                             const ProcessorConfig & /*config*/) {
    TranscriptionRow row;
    QString error;
    if (!TextGridToCsv::extractFromTextGrid(filePath, row, error))
        return Result<void>::Error(error.toStdString());

    // Convert phSeq + phDur → phoneme layer boundaries
    const QStringList phones = row.phSeq.split(' ', Qt::SkipEmptyParts);
    const QStringList durs = row.phDur.split(' ', Qt::SkipEmptyParts);

    if (phones.size() != durs.size())
        return Result<void>::Error("ph_seq and ph_dur count mismatch");

    nlohmann::json boundaries = nlohmann::json::array();
    TimePos cumPos = 0;
    for (int i = 0; i < phones.size(); ++i) {
        boundaries.push_back({{"id", i + 1}, {"pos", cumPos}, {"text", phones[i].toStdString()}});
        cumPos += secToUs(durs[i].toDouble());
    }
    // Final boundary
    boundaries.push_back({{"id", phones.size() + 1}, {"pos", cumPos}, {"text", ""}});

    layers[QStringLiteral("phoneme")] = {{"name", "phoneme"}, {"boundaries", boundaries}};

    // ph_num layer
    if (!row.phNum.isEmpty()) {
        const QStringList nums = row.phNum.split(' ', Qt::SkipEmptyParts);
        nlohmann::json phNumArr = nlohmann::json::array();
        for (const auto &n : nums)
            phNumArr.push_back(n.toInt());
        layers[QStringLiteral("ph_num")] = phNumArr;
    }

    return Result<void>::Ok();
}

Result<void> TextGridAdapter::exportFromLayers(const std::map<QString, nlohmann::json> & /*layers*/,
                                               const QString & /*outputPath*/,
                                               const ProcessorConfig & /*config*/) {
    return Result<void>::Error("TextGrid export not implemented");
}

} // namespace dstools
