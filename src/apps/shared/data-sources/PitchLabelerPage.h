#pragma once

#include "EditorPageBase.h"
#include <dstools/PhNumCalculator.h>

#include <PitchEditor.h>

namespace Rmvpe {
class Rmvpe;
}

namespace Game {
class Game;
struct GameNote;
}

namespace dstools {

class IModelManager;

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
    void onDeactivatedImpl() override;

private:
    pitchlabeler::PitchEditor *m_editor = nullptr;
    IModelManager *m_modelManager = nullptr;
    std::shared_ptr<pitchlabeler::DSFile> m_currentFile;

    QAction *m_extractPitchAction = nullptr;
    QAction *m_extractMidiAction = nullptr;

    Rmvpe::Rmvpe *m_rmvpe = nullptr;
    Game::Game *m_game = nullptr;
    PhNumCalculator m_phNumCalc;
    bool m_inferRunning = false;

    void onExtractPitch();
    void onExtractMidi();
    void onBatchExtract();
    void onAddPhNum();

    void ensureRmvpeEngine();
    void ensureGameEngine();
    void onModelInvalidated(const QString &taskKey);
    void runPitchExtraction(const QString &sliceId);
    void runMidiTranscription(const QString &sliceId);
    void runAddPhNum(const QString &sliceId);
    void applyPitchResult(const QString &sliceId, const std::vector<int32_t> &f0, float timestep);
    void applyMidiResult(const QString &sliceId, const std::vector<Game::GameNote> &notes);
};

} // namespace dstools
