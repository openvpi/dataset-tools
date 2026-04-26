#pragma once

#include "EditorPageBase.h"

#include <PitchEditor.h>
#include <atomic>
#include <dstools/PhNumCalculator.h>
#include <functional>
#include <memory>
#include <dstools/DsPitchDocument.h>

namespace Rmvpe {
    class Rmvpe;
}

namespace Game {
    class Game;
    struct GameNote;
    struct AlignInput;
}

namespace dstools {

    namespace pitchlabeler {

    }

    class PitchLabelerPage : public EditorPageBase {
        Q_OBJECT

    public:
        explicit PitchLabelerPage(QWidget *parent = nullptr);
        ~PitchLabelerPage() override;

        [[nodiscard]] QToolBar *toolbar() const {
            return m_editor->toolbar();
        }

    protected:
        QMenuBar *createMenuBar(QWidget *parent) override;
        QWidget *createStatusBarContent(QWidget *parent) override;

    protected:
        QString windowTitlePrefix() const override;
        bool isDirty() const override;
        bool saveCurrentSlice() override;
        void onSliceSelectedImpl(const QString &sliceId) override;
        void onAutoInferPreloadEngines() override;
        void onAutoInferProcessDirty(const QStringList &dirty) override;
        void onDeactivatedImpl() override;
        void saveExtraSplitters() override;
        void restoreExtraSplitters() override;
        BatchSliceResult processSlice(const QString &sliceId) override;

    private:
        pitchlabeler::PitchEditor *m_editor = nullptr;
        std::shared_ptr<pitchlabeler::DsPitchDocument> m_currentFile;

        QAction *m_extractPitchAction = nullptr;
        QAction *m_extractMidiAction = nullptr;

        Rmvpe::Rmvpe *m_rmvpe = nullptr;
        Game::Game *m_game = nullptr;
        PhNumCalculator m_phNumCalc;

        void onExtractPitch();
        void onExtractMidi();
        void onBatchExtract();
        void onAddPhNum();
        bool buildAlignInput(Game::AlignInput &outAlignInput) const;

        void onEngineInvalidated(const QString &taskKey) override;
        void runPitchExtraction(const QString &sliceId);
        void runMidiTranscription(const QString &sliceId, const Game::AlignInput *alignInput = nullptr);
        void runAddPhNum(const QString &sliceId);
        bool resolveAlignInputWithPhNum(Game::AlignInput &alignInput);
        void updateProgress();

    public:
        Rmvpe::Rmvpe *&rmvpeRef() {
            return m_rmvpe;
        }
        Game::Game *&gameRef() {
            return m_game;
        }
        QAction *extractPitchAction() const {
            return m_extractPitchAction;
        }
        QAction *extractMidiAction() const {
            return m_extractMidiAction;
        }
        void loadPhNumCalculator();
        PhNumCalculator phNumCalc() const {
            return m_phNumCalc;
        }
        void applyPitchResult(const QString &sliceId, const std::vector<float> &f0, float timestep);
        void applyMidiResult(const QString &sliceId, const std::vector<Game::GameNote> &notes);

        // Public accessors for helper functions in PitchLabelerPageHelpers
        template <typename EngineType>
        void ensureEngine(EngineType *&enginePtr, const QString &taskKey) {
            EditorPageBase::ensureEngine(enginePtr, taskKey);
        }
        EngineAliveToken &aliveToken(const QString &key);
        bool isBatchRunning() const;
        void setBatchRunning(bool running);

    private:
    };

} // namespace dstools
