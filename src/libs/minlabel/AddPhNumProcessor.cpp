#include "AddPhNumProcessor.h"

#include <dstools/PhNumCalculator.h>

#include <dsfw/TaskProcessorRegistry.h>

namespace dstools {

// Self-register with the task processor registry.
static TaskProcessorRegistry::Registrar<AddPhNumProcessor> s_reg(
    QStringLiteral("add_ph_num"), QStringLiteral("add-ph-num"));

TaskSpec AddPhNumProcessor::taskSpec() const {
    return {"add_ph_num", {{"phoneme", "phoneme"}}, {{"ph_num", "ph_num"}}};
}

Result<void> AddPhNumProcessor::initialize(IModelManager & /*mm*/,
                                           const ProcessorConfig & /*config*/) {
    return Ok();
}

void AddPhNumProcessor::release() {}

Result<TaskOutput> AddPhNumProcessor::process(const TaskInput &input) {
    auto it = input.layers.find("phoneme");
    if (it == input.layers.end())
        return Err<TaskOutput>("Missing phoneme input layer");

    const auto &phonemeLayer = it->second;

    // Build ph_seq from boundary texts.
    QString phSeq;
    if (phonemeLayer.contains("boundaries")) {
        for (const auto &bd : phonemeLayer["boundaries"]) {
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
    if (input.config.contains("dictPath")) {
        const QString dictPath =
            QString::fromStdString(input.config["dictPath"].get<std::string>());
        QString loadError;
        if (!calc.loadDictionary(dictPath, loadError))
            return Err<TaskOutput>(
                "Failed to load dictionary: " + loadError.toStdString());
    }

    QString phNum;
    QString calcError;
    if (!calc.calculate(phSeq, phNum, calcError))
        return Err<TaskOutput>("PhNum calculation failed: " + calcError.toStdString());

    // Convert ph_num string to JSON array of ints.
    nlohmann::json phNumArray = nlohmann::json::array();
    for (const auto &s : phNum.split(' ', Qt::SkipEmptyParts))
        phNumArray.push_back(s.toInt());

    TaskOutput output;
    output.layers["ph_num"] = {{"values", phNumArray}};
    return Ok(std::move(output));
}

} // namespace dstools
