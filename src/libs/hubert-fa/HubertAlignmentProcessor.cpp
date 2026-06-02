#include "HubertAlignmentProcessor.h"

#include <hubert-infer/Hfa.h>

#include <dsfw/ConfigTypes.h>
#include <dsfw/Log.h>
#include <dsfw/PathUtils.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ExecutionProvider.h>

namespace dstools {

    static TaskProcessorRegistry::Registrar<HubertAlignmentProcessor> s_reg(QStringLiteral("phoneme_alignment"),
                                                                            QStringLiteral("hubert-fa"));

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

    ProcessorConfig HubertAlignmentProcessor::capabilities() const noexcept {
        ProcessorConfig cap;
        cap["language"] = ConfigValue(false);
        cap["nonSpeechPhonemes"] = ConfigValue(std::vector<QString>{});
        return cap;
    }

    Result<void> HubertAlignmentProcessor::initialize(ModelManager & /*mm*/, const ProcessorConfig &modelConfig) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto path = configValueString(modelConfig, QStringLiteral("path")).toStdString();
        auto deviceId = static_cast<int>(configValueInt(modelConfig, QStringLiteral("deviceId"), -1));

        if (modelConfig.contains(QStringLiteral("language")))
            m_language = configValueString(modelConfig, QStringLiteral("language")).toStdString();

        if (modelConfig.contains(QStringLiteral("nonSpeechPhonemes"))) {
            auto sl = configValueStringList(modelConfig, QStringLiteral("nonSpeechPhonemes"));
            m_nonSpeechPh.clear();
            for (const auto &s : sl)
                m_nonSpeechPh.push_back(s.toStdString());
        }

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

        if (input.config.contains(QStringLiteral("language")))
            language = configValueString(input.config, QStringLiteral("language")).toStdString();
        if (input.config.contains(QStringLiteral("nonSpeechPhonemes"))) {
            auto sl = configValueStringList(input.config, QStringLiteral("nonSpeechPhonemes"));
            nonSpeechPh.clear();
            for (const auto &s : sl)
                nonSpeechPh.push_back(s.toStdString());
        }

        // Extract lyrics text from grapheme input layer
        std::string lyricsText;
        if (input.layers.count(QStringLiteral("grapheme"))) {
            auto graphemeJson = input.layers.at(QStringLiteral("grapheme")).toJson();
            for (const auto &item : graphemeJson) {
                if (!lyricsText.empty())
                    lyricsText += " ";
                lyricsText += item.get<std::string>();
            }
        }

        if (lyricsText.empty()) {
            return Result<TaskOutput>::Error("No grapheme/lyrics text provided for forced alignment");
        }

        DSFW_LOG_INFO("fa", ("FA process started | language: " + language
                              + " | lyrics: " + lyricsText).c_str());

        HFA::WordList words;
        auto recognizeResult =
            m_hfa->recognize(dsfw::PathUtils::toStdPath(input.audioPath), language, nonSpeechPh, lyricsText, words);
        if (!recognizeResult) {
            DSFW_LOG_ERROR("fa", ("FA process failed: " + recognizeResult.error()).c_str());
            return Result<TaskOutput>::Error(recognizeResult.error());
        }

        // Convert results to output layer
        nlohmann::json phonemeArray = nlohmann::json::array();
        for (const auto &word : words) {
            for (const auto &phone : word.phones) {
                phonemeArray.push_back({
                    {"phone", phone.text },
                    {"start", phone.start},
                    {"end",   phone.end  },
                });
            }
        }

        DSFW_LOG_INFO("fa", ("FA process completed | phonemes: " + std::to_string(phonemeArray.size())).c_str());

        TaskOutput output;
        output.layers[QStringLiteral("phoneme")] = LayerData::fromJson(std::move(phonemeArray));
        return Result<TaskOutput>::Ok(std::move(output));
    }

} // namespace dstools
