#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>
#include <dstools/PhNumCalculator.h>

#include <QWidget>
#include <QSplitter>

#include <PitchEditor.h>

namespace Rmvpe {
class Rmvpe;
}

namespace Game {
class Game;
struct GameNote;
}

namespace dstools {

class IEditorDataSource;
class ISettingsBackend;
class SliceListPanel;
class IModelManager;

namespace pitchlabeler {
class DSFile;
}

class PitchLabelerPage : public QWidget,
                         public labeler::IPageActions,
                         public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit PitchLabelerPage(QWidget *parent = nullptr);
    ~PitchLabelerPage() override;

    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);

    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onShutdown() override;

    dstools::widgets::ShortcutManager *shortcutManager() const;

signals:
    void sliceChanged(const QString &sliceId);

private:
    pitchlabeler::PitchEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    QSplitter *m_splitter = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    IModelManager *m_modelManager = nullptr;
    QString m_currentSliceId;
    std::shared_ptr<pitchlabeler::DSFile> m_currentFile;

    dstools::AppSettings m_settings;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;
    QAction *m_extractPitchAction = nullptr;
    QAction *m_extractMidiAction = nullptr;

    Rmvpe::Rmvpe *m_rmvpe = nullptr;
    Game::Game *m_game = nullptr;
    PhNumCalculator m_phNumCalc;
    bool m_inferRunning = false;

    void onSliceSelected(const QString &sliceId);
    bool saveCurrentSlice();
    bool maybeSave();

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
