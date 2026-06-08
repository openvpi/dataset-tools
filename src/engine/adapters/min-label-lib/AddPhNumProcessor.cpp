#include "AddPhNumProcessor.h"

#include <dstools/DsKeys.h>
#include <dstools/PhNumCalculator.h>

#include <dsfw/ConfigTypes.h>
#include <dsfw/TaskProcessorRegistry.h>

namespace dstools {



// Self-register with the task processor registry.
static TaskProcessorRegistry::Registrar<dsfw::AddPhNumProcessor> s_reg(
    QStringLiteral("add_ph_num"), QStringLiteral("add-ph-num"));

TaskSpec AddPhNumProcessor::taskSpec() const {
    return {"add_ph_num",
            {{QString::fromUtf8(dstools::keys::layers::phoneme), QString::fromUtf8(dstools::keys::layers::phoneme)}},
            {{QString::fromUtf8(dstools::keys::layers::phNum), QString::fromUtf8(dstools::keys::layers::phNum)}}};
}

dsfw::Result<void> AddPhNumProcessor::initialize(dsfw::ModelManager & /*mm*/,
                                           const dsfw::ProcessorConfig & /*config*/) {
    return dsfw::Ok();
}

void AddPhNumProcessor::release() {}

dsfw::Result<dsfw::TaskOutput> AddPhNumProcessor::process(const dsfw::TaskInput &input) {
    auto it = input.layers.find(QString::fromUtf8(dstools::keys::layers::phoneme));
    if (it == input.layers.end())
        return dsfw::Err<dsfw::TaskOutput>("Missing phoneme input layer");

    const auto &phonemeLayerData = it->second;

    // Build ph_seq from boundary texts.
    QString phSeq;
    auto phonemeJson = phonemeLayerData.toJson();
    if (phonemeJson.contains("boundaries")) {
        for (const auto &bd : phonemeJson["boundaries"]) {
            const QString text = QString::fromStdString(bd.value("text", std::string{}));
            if (!text.isEmpty()) {
                if (!phSeq.isEmpty())
                    phSeq += ' ';
                phSeq += text;
            }
        }
    }

    if (phSeq.isEmpty())
        return dsfw::Err<dsfw::TaskOutput>("Empty phoneme sequence");

    dsfw::PhNumCalculator calc;

    // Load dictionary if path provided in config.
    if (input.config.contains(QStringLiteral("dictPath"))) {
        const QString dictPath = configValueString(input.config, QStringLiteral("dictPath"));
        auto loadResult = calc.loadDictionary(dictPath);
        if (!loadResult.ok())
            return dsfw::Err<dsfw::TaskOutput>(
                "Failed to load dictionary: " + loadResult.error());
    }

    auto calcResult = calc.calculate(phSeq);
    if (!calcResult.ok())
        return dsfw::Err<dsfw::TaskOutput>("PhNum calculation failed: " + calcResult.error());

    const QString phNum = std::move(calcResult.value());

    // Convert ph_num string to JSON array of ints.
    nlohmann::json phNumArray = nlohmann::json::array();
    for (const auto &s : phNum.split(' ', Qt::SkipEmptyParts))
        phNumArray.push_back(s.toInt());

    dsfw::TaskOutput output;
    output.layers[QString::fromUtf8(dstools::keys::layers::phNum)] = LayerData::fromJson({{"values", phNumArray}});
    return dsfw::Ok(std::move(output));
}

} // namespace dstools
