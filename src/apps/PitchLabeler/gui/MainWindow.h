#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QLabel>
#include <QSplitter>
#include <QStackedWidget>
#include <QListWidget>
#include <QShortcut>
#include <QSlider>
#include <QSize>

#include <dstools/AppSettings.h>
#include <dstools/Theme.h>
#include <dstools/PlayWidget.h>
#include <dstools/ShortcutManager.h>

#include "ui/PianoRollView.h"

#include <memory>

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {
class PianoRollView;
class PropertyPanel;
class FileListPanel;
}

/// Main application window for DiffSinger Pitch Labeler
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // Settings
    dstools::AppSettings m_settings{"DiffSingerPitchLabeler"};

    // File management
    QString m_workingDirectory;
    QString m_currentFilePath;
    std::shared_ptr<DSFile> m_currentFile;

    // Services
    dstools::widgets::PlayWidget *m_playWidget = nullptr;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    // UI Components - Menu
    QMenu *m_fileMenu = nullptr;
    QMenu *m_editMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QMenu *m_helpMenu = nullptr;

    // Menu actions - File
    QAction *m_actOpenDir = nullptr;
    QAction *m_actCloseDir = nullptr;
    QAction *m_actSave = nullptr;
    QAction *m_actSaveAll = nullptr;
    QAction *m_actExit = nullptr;

    // Menu actions - Edit
    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;

    // Menu actions - View
    QAction *m_actZoomIn = nullptr;
    QAction *m_actZoomOut = nullptr;
    QAction *m_actZoomReset = nullptr;

    // Menu actions - Tools
    QAction *m_actABCompare = nullptr;

    // Menu actions - Help
    QAction *m_actAbout = nullptr;

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

    // Status bar
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusPosition = nullptr;
    QLabel *m_statusZoom = nullptr;
    QLabel *m_statusNotes = nullptr;
    QLabel *m_statusTool = nullptr;

    // State
    bool m_abComparisonActive = false;
    std::vector<double> m_originalF0;
    QList<QShortcut *> m_windowShortcuts;

    // Helpers
    void buildMenuBar();
    void buildToolbar();
    void buildCentralLayout();
    void buildStatusBar();

    void connectSignals();
    void applyConfig();

    // File operations
    void openDirectory();
    void closeDirectory();
    void loadFile(const QString &path);
    bool saveFile();
    bool saveAllFiles();
    void updateWindowTitle();

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
    void updateStatusBar();
    void reloadCurrentFile();
    void rebuildWindowShortcuts();

signals:
    void fileLoaded(const QString &path);
    void fileSaved(const QString &path);
};

} // namespace pitchlabeler
} // namespace dstools
