#include "GameMidiProcessor.h"

#include <game-infer/Game.h>

#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ExecutionProvider.h>

namespace dstools {

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

ProcessorConfig GameMidiProcessor::capabilities() const {
    return {
        {"segThreshold",   {{"type", "float"}, {"min", 0.0},  {"max", 1.0},   {"default", 0.2}}},
        {"segRadiusFrames",{{"type", "float"}, {"min", 0},    {"max", 50},    {"default", 2.0}}},
        {"estThreshold",   {{"type", "float"}, {"min", 0.0},  {"max", 1.0},   {"default", 0.2}}},
        {"d3pmTimesteps",  {{"type", "int"},   {"min", 1},    {"max", 1000},  {"default", 100}}},
        {"language",       {{"type", "int"},   {"default", 0}}}
    };
}

Result<void> GameMidiProcessor::initialize(IModelManager & /*mm*/,
                                           const ProcessorConfig &modelConfig) {
    std::lock_guard lock(m_mutex);

    if (!m_game) {
        m_game = std::make_shared<Game::Game>();
    }

    const auto path = modelConfig.value("path", std::string{});
    const auto deviceId = modelConfig.value("deviceId", -1);

    auto provider = deviceId < 0 ? Game::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : Game::ExecutionProvider::DML;
#else
                                 : Game::ExecutionProvider::CPU;
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

    if (config.contains("segThreshold")) {
        m_game->setSegThreshold(config["segThreshold"].get<float>());
    }
    if (config.contains("segRadiusFrames")) {
        m_game->setSegRadiusFrames(config["segRadiusFrames"].get<float>());
    }
    if (config.contains("estThreshold")) {
        m_game->setEstThreshold(config["estThreshold"].get<float>());
    }
    if (config.contains("d3pmTimesteps")) {
        const auto nSteps = config["d3pmTimesteps"].get<int>();
        if (nSteps > 0) {
            m_game->setD3pmTs(generateD3pmTimesteps(nSteps));
        }
    }
    if (config.contains("language")) {
        m_game->setLanguage(config["language"].get<int>());
    }
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

    if (input.config.contains("csvPath")) {
        const auto csvPath = QString::fromStdString(input.config["csvPath"].get<std::string>());
        const auto savePath = input.config.value("savePath", input.config["csvPath"].get<std::string>());
        Game::AlignOptions opts;
        auto result = m_game->alignCSV(csvPath.toStdWString(),
                                       QString::fromStdString(savePath).toStdWString(),
                                       "", false, opts, nullptr);
        if (!result) {
            return Err<TaskOutput>(result.error());
        }
        TaskOutput output;
        output.layers["alignment"] = nlohmann::json::object({
            {"csvPath", csvPath.toStdString()},
            {"savePath", savePath}
        });
        return Ok(std::move(output));
    }

    std::vector<Game::GameNote> notes;
    auto result = m_game->getNotes(input.audioPath.toStdWString(), notes, nullptr);
    if (!result) {
        return Err<TaskOutput>(result.error());
    }

    TaskOutput output;
    auto &midiLayer = output.layers["midi"];
    midiLayer = nlohmann::json::array();
    for (const auto &n : notes) {
        midiLayer.push_back({
            {"pitch",    n.pitch},
            {"onset",    n.onset},
            {"duration", n.duration},
            {"voiced",   n.voiced}
        });
    }
    return Ok(std::move(output));
}

} // namespace dstools
