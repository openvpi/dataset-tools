#pragma once

#include "EditorPageBase.h"
#include <dstools/PhNumCalculator.h>

#include <atomic>
#include <functional>
#include <memory>

#include <PitchEditor.h>

namespace Rmvpe {
class Rmvpe;
}

namespace Game {
class Game;
struct GameNote;
struct AlignInput;
}

template<typename EngineType>
struct EngineTraits;

namespace dstools {

namespace pitchlabeler {
class DSFile;
}

class PitchLabelerPage : public EditorPageBase {
    Q_OBJECT

public:
    explicit PitchLabelerPage(QWidget *parent = nullptr);
    ~PitchLabelerPage() override;

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;

protected:
    QString windowTitlePrefix() const override;
    bool isDirty() const override;
    bool saveCurrentSlice() override;
    void onSliceSelectedImpl(const QString &sliceId) override;
    void onAutoInfer() override;
    void onDeactivatedImpl() override;

    // Batch processing (P-12 template)
    bool isBatchRunning() const override;
    void setBatchRunning(bool running) override;
    std::shared_ptr<std::atomic<bool>> batchAliveToken() const override;

private:
    pitchlabeler::PitchEditor *m_editor = nullptr;
    std::shared_ptr<pitchlabeler::DSFile> m_currentFile;

    QAction *m_extractPitchAction = nullptr;
    QAction *m_extractMidiAction = nullptr;

    Rmvpe::Rmvpe *m_rmvpe = nullptr;
    Game::Game *m_game = nullptr;
    PhNumCalculator m_phNumCalc;
    std::atomic<bool> m_inferRunning{false};
    std::atomic<bool> m_batchRunning{false};
    std::shared_ptr<std::atomic<bool>> m_batchAlive;

    void onExtractPitch();
    void onExtractMidi();
    void onBatchExtract();
    void onAddPhNum();
    void loadPhNumCalculator();

    template<typename EngineType>
    void ensureEngine(EngineType *&enginePtr, EngineAliveToken &aliveToken, const QString &taskKey);

    template<typename EngineType>
    void ensureEngineAsync(EngineType *&enginePtr, EngineAliveToken &aliveToken, const QString &taskKey,
                           std::function<void()> onReady);

    void ensureRmvpeEngine();
    void ensureGameEngine();
    void ensureRmvpeEngineAsync(std::function<void()> onReady = {});
    void ensureGameEngineAsync(std::function<void()> onReady = {});
    void onModelInvalidated(const QString &taskKey);
    void runPitchExtraction(const QString &sliceId);
    void runMidiTranscription(const QString &sliceId, const Game::AlignInput *alignInput = nullptr);
    void runAddPhNum(const QString &sliceId);
    void applyPitchResult(const QString &sliceId, const std::vector<int32_t> &f0, float timestep);
    void applyMidiResult(const QString &sliceId, const std::vector<Game::GameNote> &notes);
    void updateProgress();
};

} // namespace dstools
