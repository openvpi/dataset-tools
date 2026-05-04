#include "HubertAlignmentProcessor.h"

#include <hubert-infer/Hfa.h>

#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ExecutionProvider.h>

namespace dstools {

static TaskProcessorRegistry::Registrar<HubertAlignmentProcessor> s_reg(
    QStringLiteral("phoneme_alignment"), QStringLiteral("hubert-fa"));

HubertAlignmentProcessor::HubertAlignmentProcessor() = default;
HubertAlignmentProcessor::~HubertAlignmentProcessor() = default;

QString HubertAlignmentProcessor::processorId() const {
    return QStringLiteral("hubert-fa");
}

QString HubertAlignmentProcessor::displayName() const {
    return QStringLiteral("HuBERT Forced Alignment");
}

TaskSpec HubertAlignmentProcessor::taskSpec() const {
    return {QStringLiteral("phoneme_alignment"),
            {{QStringLiteral("grapheme"), QStringLiteral("grapheme")}},
            {{QStringLiteral("phoneme"), QStringLiteral("phoneme")}}};
}

ProcessorConfig HubertAlignmentProcessor::capabilities() const {
    return {
        {"language",
         {{"type", "enum"}, {"values", {"zh", "ja", "en"}}, {"default", "zh"}}},
        {"nonSpeechPhonemes",
         {{"type", "stringList"}, {"default", {"AP", "SP"}}}},
    };
}

Result<void> HubertAlignmentProcessor::initialize(IModelManager & /*mm*/,
                                                   const ProcessorConfig &modelConfig) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto path = modelConfig.value("path", std::string{});
    auto deviceId = modelConfig.value("deviceId", -1);

    if (modelConfig.contains("language"))
        m_language = modelConfig["language"].get<std::string>();

    if (modelConfig.contains("nonSpeechPhonemes"))
        m_nonSpeechPh = modelConfig["nonSpeechPhonemes"].get<std::vector<std::string>>();

    auto provider = deviceId < 0 ? dstools::infer::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : dstools::infer::ExecutionProvider::DML;
#else
                                 : dstools::infer::ExecutionProvider::CPU;
#endif

    m_hfa = std::make_unique<HFA::HFA>();
    auto result = m_hfa->load(path, provider, deviceId);
    if (!result) {
        m_hfa.reset();
        return Err(result.error());
    }
    return Ok();
}

void HubertAlignmentProcessor::release() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hfa.reset();
}

Result<TaskOutput> HubertAlignmentProcessor::process(const TaskInput &input) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_hfa || !m_hfa->isOpen()) {
        return Result<TaskOutput>::Error("HuBERT-FA model is not loaded");
    }

    // Per-call config overrides
    auto language = m_language;
    auto nonSpeechPh = m_nonSpeechPh;

    if (input.config.contains("language"))
        language = input.config["language"].get<std::string>();
    if (input.config.contains("nonSpeechPhonemes"))
        nonSpeechPh = input.config["nonSpeechPhonemes"].get<std::vector<std::string>>();

    // Extract lyrics text from grapheme input layer
    std::string lyricsText;
    if (input.layers.count(QStringLiteral("grapheme"))) {
        const auto &graphemeLayer = input.layers.at(QStringLiteral("grapheme"));
        for (const auto &item : graphemeLayer) {
            if (!lyricsText.empty())
                lyricsText += " ";
            lyricsText += item.get<std::string>();
        }
    }

    if (lyricsText.empty()) {
        return Result<TaskOutput>::Error("No grapheme/lyrics text provided for forced alignment");
    }

    HFA::WordList words;
    auto recognizeResult = m_hfa->recognize(
        input.audioPath.toLocal8Bit().toStdString(), language, nonSpeechPh, lyricsText, words);
    if (!recognizeResult) {
        return Result<TaskOutput>::Error(recognizeResult.error());
    }

    // Convert results to output layer
    nlohmann::json phonemeArray = nlohmann::json::array();
    for (const auto &word : words) {
        for (const auto &phone : word.phones) {
            phonemeArray.push_back({
                {"phone", phone.text},
                {"start", phone.start},
                {"end",   phone.end  },
            });
        }
    }

    TaskOutput output;
    output.layers[QStringLiteral("phoneme")] = std::move(phonemeArray);
    return Result<TaskOutput>::Ok(std::move(output));
}

} // namespace dstools
