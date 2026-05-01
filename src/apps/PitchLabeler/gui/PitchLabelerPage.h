#pragma once

#include <dsfw/IPageActions.h>

#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolButton>
#include <QProxyStyle>
#include <QShortcut>
#include <QUndoStack>

#include <dsfw/AppSettings.h>
#include <dstools/PlayWidget.h>
#include <dstools/ShortcutManager.h>
#include <dstools/ViewportController.h>

#include "ui/PianoRollView.h"

#include <memory>

namespace dstools {
namespace pitchlabeler {

class DSFile;
class PitchFileService;

namespace ui {
class PianoRollView;
class PropertyPanel;
class FileListPanel;
}

/// Reusable page widget containing all PitchLabeler UI and logic.
/// MainWindow becomes a thin shell providing menus, status bar, and window chrome.
class PitchLabelerPage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit PitchLabelerPage(QWidget *parent = nullptr);
    ~PitchLabelerPage() override;

    void setWorkingDirectory(const QString &dir) override;
    void closeDirectory();
    [[nodiscard]] QString workingDirectory() const override { return m_workingDirectory; }

    [[nodiscard]] bool hasUnsavedChanges() const override;
    bool save();
    bool saveAll();

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;

    void openDirectory();

    // Access for MainWindow shortcut binding
    [[nodiscard]] QAction *saveAction() const { return m_actSave; }
    [[nodiscard]] QAction *saveAllAction() const { return m_actSaveAll; }
    [[nodiscard]] QAction *undoAction() const { return m_actUndo; }
    [[nodiscard]] QAction *redoAction() const { return m_actRedo; }
    [[nodiscard]] QAction *zoomInAction() const { return m_actZoomIn; }
    [[nodiscard]] QAction *zoomOutAction() const { return m_actZoomOut; }
    [[nodiscard]] QAction *zoomResetAction() const { return m_actZoomReset; }
    [[nodiscard]] QAction *abCompareAction() const { return m_actABCompare; }

    [[nodiscard]] dstools::widgets::ShortcutManager *shortcutManager() const { return m_shortcutManager; }
    [[nodiscard]] dstools::AppSettings &settings() { return m_settings; }

    // Config persistence
    void saveConfig();

signals:
    void workingDirectoryChanged(const QString &dir);
    void fileSelected(const QString &path);
    void modificationChanged(bool modified);
    void toolModeChanged(int mode);
    void positionChanged(double sec);
    void zoomChanged(int percent);
    void noteCountChanged(int count);
    void fileStatusChanged(const QString &fileName);

    void fileLoaded(const QString &path);
    void fileSaved(const QString &path);

private:
    // Settings
    dstools::AppSettings m_settings{"DiffSingerPitchLabeler"};

    // File management
    QString m_workingDirectory;
    PitchFileService *m_fileService = nullptr;
    std::shared_ptr<DSFile> m_currentFile;
    QString m_currentFilePath;

    // Services
    dstools::widgets::ViewportController *m_viewport = nullptr;
    dstools::widgets::PlayWidget *m_playWidget = nullptr;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    // Actions - Edit
    QAction *m_actSave = nullptr;
    QAction *m_actSaveAll = nullptr;
    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;

    // Actions - View
    QAction *m_actZoomIn = nullptr;
    QAction *m_actZoomOut = nullptr;
    QAction *m_actZoomReset = nullptr;

    // Actions - Tools
    QAction *m_actABCompare = nullptr;

    // Toolbar actions
    QAction *m_actPlayPause = nullptr;
    QAction *m_actStop = nullptr;

    // Tool mode buttons
    QToolButton *m_btnToolSelect = nullptr;
    QToolButton *m_btnToolModulation = nullptr;
    QToolButton *m_btnToolDrift = nullptr;
    QToolButton *m_btnToolAudition = nullptr;
    QActionGroup *m_toolModeGroup = nullptr;

    // Waveform / volume controls
    QToolButton *m_btnWaveformToggle = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_volumeLabel = nullptr;

    // Playback progress widget above piano roll
    QWidget *m_playbackProgressWidget = nullptr;
    QSlider *m_playbackProgressSlider = nullptr;
    QLabel *m_progressCurrentTime = nullptr;
    QLabel *m_progressTotalTime = nullptr;

    // UI Components - Panels
    QWidget *m_emptyPage = nullptr;
    QSplitter *m_outerSplitter = nullptr;
    QSplitter *m_rightSplitter = nullptr;
    QStackedWidget *m_mainStack = nullptr;
    QStackedWidget *m_viewStack = nullptr;

    ui::FileListPanel *m_fileListPanel = nullptr;
    ui::PianoRollView *m_pianoRoll = nullptr;
    ui::PropertyPanel *m_propertyPanel = nullptr;

    // State
    bool m_abComparisonActive = false;
    std::vector<double> m_originalF0;
    QList<QShortcut *> m_windowShortcuts;
    QUndoStack *m_undoStack = nullptr;
    std::unique_ptr<QProxyStyle> m_jumpClickStyle;

    // Helpers
    void buildLayout();
    void buildActions();
    void connectSignals();
    void applyConfig();

    // File operations
    void onFileLoaded(const QString &path, std::shared_ptr<DSFile> file);
    void onFileSaved(const QString &path);
    bool saveFile();
    bool saveAllFiles();

    // File navigation
    void onNextFile();
    void onPrevFile();

    // Edit operations
    void onUndo();
    void onRedo();
    void updateUndoRedoState();

    // View operations
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void updateZoomStatus();

    // Playback
    void onPlayPause();
    void onStop();
    void updatePlayheadPosition(double sec);
    void updatePlaybackState();
    void updateTimeLabels(double sec);

    // Tool mode
    void setToolMode(ui::ToolMode mode);

    // Selection
    void onNoteSelected(int noteIndex);
    void onPositionClicked(double time, double midi);

    // Updates
    void updateStatusInfo();
    void reloadCurrentFile();
    void rebuildWindowShortcuts();
};

} // namespace pitchlabeler
} // namespace dstools
