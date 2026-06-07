#include "GameMidiProcessor.h"

#include <game-infer/Game.h>

#include <dsfw/ConfigTypes.h>
#include <dsfw/PathUtils.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/infer/ExecutionProvider.h>

namespace dstools {

using namespace dsfw;

using namespace dsfw;

// Self-register with the task processor registry.
static TaskProcessorRegistry::Registrar<GameMidiProcessor> s_reg(
    QStringLiteral("midi_transcription"), QStringLiteral("game"));

GameMidiProcessor::GameMidiProcessor() = default;
GameMidiProcessor::~GameMidiProcessor() = default;

QString GameMidiProcessor::processorId() const {
    return QStringLiteral("game");
}

QString GameMidiProcessor::displayName() const {
    return QStringLiteral("GAME Audio-to-MIDI");
}

TaskSpec GameMidiProcessor::taskSpec() const {
    return {"midi_transcription", {}, {{"midi", "midi"}}};
}

ProcessorConfig GameMidiProcessor::capabilities() const noexcept {
        ProcessorConfig cap;
        cap["segThreshold"] = ConfigValue(0.0);
        cap["segRadiusFrames"] = ConfigValue(0.0);
        cap["estThreshold"] = ConfigValue(0.0);
        cap["d3pmTimesteps"] = ConfigValue(int64_t(0));
        cap["language"] = ConfigValue(int64_t(0));
        return cap;
    }

Result<void> GameMidiProcessor::initialize(ModelManager & /*mm*/,
                                           const ProcessorConfig &modelConfig) {
    std::lock_guard lock(m_mutex);

    if (!m_game) {
        m_game = std::make_shared<Game::Game>();
    }

    const auto path = configValueString(modelConfig, QStringLiteral("path")).toStdString();
    const auto deviceId = static_cast<int>(configValueInt(modelConfig, QStringLiteral("deviceId"), -1));

    auto provider = deviceId < 0 ? dsfw::infer::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : dsfw::infer::ExecutionProvider::DML;
#else
                                 : dsfw::infer::ExecutionProvider::CPU;
#endif

    auto result = m_game->loadModel(std::filesystem::path(path), provider, deviceId);
    if (!result) {
        m_game.reset();
        return Err(result.error());
    }

    return Ok();
}

void GameMidiProcessor::release() {
    std::lock_guard lock(m_mutex);
    m_game.reset();
}

void GameMidiProcessor::applyConfig(const ProcessorConfig &config) const {
        if (!m_game)
            return;

        if (config.contains(QStringLiteral("segThreshold")))
            m_game->setSegThreshold(static_cast<float>(configValueDouble(config, QStringLiteral("segThreshold"))));
        if (config.contains(QStringLiteral("segRadiusFrames")))
            m_game->setSegRadiusFrames(static_cast<float>(configValueDouble(config, QStringLiteral("segRadiusFrames"))));
        if (config.contains(QStringLiteral("estThreshold")))
            m_game->setEstThreshold(static_cast<float>(configValueDouble(config, QStringLiteral("estThreshold"))));
        if (config.contains(QStringLiteral("d3pmTimesteps"))) {
            const auto nSteps = static_cast<int>(configValueInt(config, QStringLiteral("d3pmTimesteps")));
            if (nSteps > 0)
                m_game->setD3pmTs(generateD3pmTimesteps(nSteps));
        }
        if (config.contains(QStringLiteral("language")))
            m_game->setLanguage(static_cast<int>(configValueInt(config, QStringLiteral("language"))));
    }

std::vector<float> GameMidiProcessor::generateD3pmTimesteps(int nSteps) {
    std::vector<float> ts;
    if (nSteps <= 0)
        return ts;
    constexpr float t0 = 0.0f;
    const float step = (1.0f - t0) / static_cast<float>(nSteps);
    ts.reserve(nSteps);
    for (int i = 0; i < nSteps; ++i) {
        ts.push_back(t0 + static_cast<float>(i) * step);
    }
    return ts;
}

Result<TaskOutput> GameMidiProcessor::process(const TaskInput &input) {
    std::lock_guard lock(m_mutex);

    if (!m_game || !m_game->isOpen()) {
        return Err<TaskOutput>("Model not loaded");
    }

    applyConfig(input.config);

    if (input.config.contains(QStringLiteral("csvPath"))) {
        const auto csvPath = configValueString(input.config, QStringLiteral("csvPath"));
        const auto savePath = configValueString(input.config, QStringLiteral("savePath"), csvPath);
        Game::AlignOptions opts;
        auto result = m_game->alignCSV(dsfw::PathUtils::toStdPath(csvPath),
                                       dsfw::PathUtils::toStdPath(savePath),
                                       std::string{}, false, opts, nullptr);
        if (!result) {
            return Err<TaskOutput>(result.error());
        }
        TaskOutput output;
        output.layers["alignment"] = LayerData::fromJson(nlohmann::json::object({
            {"csvPath", csvPath.toStdString()},
            {"savePath", savePath.toStdString()}
        }));
        return Ok(std::move(output));
    }

    std::vector<Game::GameNote> notes;
    auto result = m_game->getNotes(input.audioPath.toStdWString(), notes, nullptr);
    if (!result) {
        return Err<TaskOutput>(result.error());
    }

    TaskOutput output;
    auto &midiLayerData = output.layers["midi"];
    nlohmann::json midiLayer = nlohmann::json::array();
    for (const auto &n : notes) {
        midiLayer.push_back({
            {"pitch",    n.pitch},
            {"onset",    n.onset},
            {"duration", n.duration},
            {"voiced",   n.voiced}
        });
    }
    midiLayerData = LayerData::fromJson(midiLayer);
    return Ok(std::move(output));
}

} // namespace dstools
