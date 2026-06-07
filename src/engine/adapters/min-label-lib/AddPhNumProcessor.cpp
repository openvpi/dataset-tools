#include "AddPhNumProcessor.h"

#include <dstools/DsKeys.h>
#include <dstools/PhNumCalculator.h>

#include <dsfw/ConfigTypes.h>
#include <dsfw/TaskProcessorRegistry.h>

namespace dstools {

using namespace dsfw;

using namespace dsfw;

// Self-register with the task processor registry.
static TaskProcessorRegistry::Registrar<AddPhNumProcessor> s_reg(
    QStringLiteral("add_ph_num"), QStringLiteral("add-ph-num"));

TaskSpec AddPhNumProcessor::taskSpec() const {
    return {"add_ph_num",
            {{QString::fromUtf8(dstools::keys::layers::phoneme), QString::fromUtf8(dstools::keys::layers::phoneme)}},
            {{QString::fromUtf8(dstools::keys::layers::phNum), QString::fromUtf8(dstools::keys::layers::phNum)}}};
}

Result<void> AddPhNumProcessor::initialize(ModelManager & /*mm*/,
                                           const ProcessorConfig & /*config*/) {
    return Ok();
}

void AddPhNumProcessor::release() {}

Result<TaskOutput> AddPhNumProcessor::process(const TaskInput &input) {
    auto it = input.layers.find(QString::fromUtf8(dstools::keys::layers::phoneme));
    if (it == input.layers.end())
        return Err<TaskOutput>("Missing phoneme input layer");

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
        return Err<TaskOutput>("Empty phoneme sequence");

    PhNumCalculator calc;

    // Load dictionary if path provided in config.
    if (input.config.contains(QStringLiteral("dictPath"))) {
        const QString dictPath = configValueString(input.config, QStringLiteral("dictPath"));
        auto loadResult = calc.loadDictionary(dictPath);
        if (!loadResult.ok())
            return Err<TaskOutput>(
                "Failed to load dictionary: " + loadResult.error());
    }

    auto calcResult = calc.calculate(phSeq);
    if (!calcResult.ok())
        return Err<TaskOutput>("PhNum calculation failed: " + calcResult.error());

    const QString phNum = std::move(calcResult.value());

    // Convert ph_num string to JSON array of ints.
    nlohmann::json phNumArray = nlohmann::json::array();
    for (const auto &s : phNum.split(' ', Qt::SkipEmptyParts))
        phNumArray.push_back(s.toInt());

    TaskOutput output;
    output.layers[QString::fromUtf8(dstools::keys::layers::phNum)] = LayerData::fromJson({{"values", phNumArray}});
    return Ok(std::move(output));
}

} // namespace dstools
