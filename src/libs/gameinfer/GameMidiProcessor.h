#pragma once

/// @file GameMidiProcessor.h
/// @brief GAME-based audio-to-MIDI transcription task processor.

#include <dsfw/ITaskProcessor.h>

#include <memory>
#include <mutex>

namespace Game {
class Game;
}

namespace dstools {

/// @brief ITaskProcessor implementation for GAME audio-to-MIDI transcription.
///
/// Wraps the GAME engine as a single ModelTypeId (ADR-14: not split into sub-models).
/// Exposes 5 configurable parameters via capabilities() replacing dynamic_cast setters.
class GameMidiProcessor : public ITaskProcessor {
public:
    GameMidiProcessor();
    ~GameMidiProcessor() override;

    QString processorId() const override;
    QString displayName() const override;
    TaskSpec taskSpec() const override;
    ProcessorConfig capabilities() const override;

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &modelConfig) override;
    void release() override;

    Result<TaskOutput> process(const TaskInput &input) override;
    Result<BatchOutput> processBatch(const BatchInput &input, ProgressCallback progress) override;

private:
    /// @brief Generate D3PM timestep schedule (linspace from 0 to 1).
    static std::vector<float> generateD3pmTimesteps(int nSteps);

    /// @brief Apply configuration parameters to the engine.
    void applyConfig(const ProcessorConfig &config) const;

    std::shared_ptr<Game::Game> m_game;
    mutable std::mutex m_mutex;
};

} // namespace dstools
