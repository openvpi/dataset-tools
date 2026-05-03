#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/AppSettings.h>
#include <dstools/ShortcutManager.h>
#include <dstools/PhNumCalculator.h>

#include <QWidget>

#include <PitchEditor.h>

#include <memory>

namespace Rmvpe {
class Rmvpe;
}

#include <game-infer/Game.h>

namespace dstools {

class IEditorDataSource;
class ISettingsBackend;
class SliceListPanel;

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
    void onShutdown() override;

    dstools::widgets::ShortcutManager *shortcutManager() const;

signals:
    void sliceChanged(const QString &sliceId);

private:
    pitchlabeler::PitchEditor *m_editor = nullptr;
    SliceListPanel *m_sliceList = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    QString m_currentSliceId;
    std::shared_ptr<pitchlabeler::DSFile> m_currentFile;

    dstools::AppSettings m_settings;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;

    std::unique_ptr<Rmvpe::Rmvpe> m_rmvpe;
    std::unique_ptr<Game::Game> m_game;
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
    void runPitchExtraction(const QString &sliceId);
    void runMidiTranscription(const QString &sliceId);
    void runAddPhNum(const QString &sliceId);
    void applyPitchResult(const QString &sliceId, const std::vector<int32_t> &f0, float timestep);
    void applyMidiResult(const QString &sliceId, const std::vector<Game::GameNote> &notes);
};

} // namespace dstools
