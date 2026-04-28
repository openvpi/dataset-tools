#pragma once

#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QSplitter>
#include <QUndoStack>
#include <QScopedPointer>

#include <dstools/AppSettings.h>
#include <dstools/PlayWidget.h>
#include <dstools/ShortcutManager.h>

#include "../PhonemeLabelerKeys.h"
#include "ui/TextGridDocument.h"
#include "ui/ViewportController.h"
#include "ui/BoundaryBindingManager.h"
#include "ui/WaveformWidget.h"
#include "ui/TierEditWidget.h"
#include "ui/PowerWidget.h"
#include "ui/SpectrogramWidget.h"
#include "ui/BoundaryOverlayWidget.h"
#include "ui/FileListPanel.h"
#include "ui/EntryListPanel.h"
#include "ui/TimeRulerWidget.h"

#include <QScrollBar>

namespace dstools {
namespace phonemelabeler {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    dstools::AppSettings m_settings{"PhonemeLabeler"};

    // Document
    TextGridDocument *m_document = nullptr;
    QUndoStack *m_undoStack = nullptr;
    ViewportController *m_viewport = nullptr;
    BoundaryBindingManager *m_bindingManager = nullptr;

    // File state
    QString m_currentDir;
    QString m_currentFilePath;
    bool m_isModified = false;

    // Services
    dstools::widgets::PlayWidget *m_playWidget = nullptr;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;

    // UI Components
    QSplitter *m_mainSplitter = nullptr;
    QSplitter *m_rightSplitter = nullptr;
    WaveformWidget *m_waveformWidget = nullptr;
    TierEditWidget *m_tierEditWidget = nullptr;
    PowerWidget *m_powerWidget = nullptr;
    SpectrogramWidget *m_spectrogramWidget = nullptr;
    FileListPanel *m_fileListPanel = nullptr;
    EntryListPanel *m_entryListPanel = nullptr;
    TimeRulerWidget *m_timeRulerWidget = nullptr;
    QScrollBar *m_hScrollBar = nullptr;
    BoundaryOverlayWidget *m_boundaryOverlay = nullptr;

    // Menu
    QMenu *m_fileMenu = nullptr;
    QMenu *m_editMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_helpMenu = nullptr;

    QAction *m_actOpenDir = nullptr;
    QAction *m_actOpenFile = nullptr;
    QAction *m_actSave = nullptr;
    QAction *m_actSaveAs = nullptr;
    QAction *m_actExit = nullptr;

    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;

    QAction *m_actZoomIn = nullptr;
    QAction *m_actZoomOut = nullptr;
    QAction *m_actZoomReset = nullptr;
    QAction *m_actToggleBinding = nullptr;
    QAction *m_actTogglePower = nullptr;
    QAction *m_actToggleSpectrogram = nullptr;
    QMenu *m_spectrogramColorMenu = nullptr;
    QActionGroup *m_spectrogramColorGroup = nullptr;

    QAction *m_actAbout = nullptr;

    // Toolbar
    QAction *m_actPlayPause = nullptr;
    QAction *m_actStop = nullptr;
    QAction *m_actBindingToggle = nullptr;

    // Status bar
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusPosition = nullptr;
    QLabel *m_statusZoom = nullptr;
    QLabel *m_statusBinding = nullptr;

    // Helpers
    void buildMenuBar();
    void buildToolbar();
    void buildCentralLayout();
    void buildStatusBar();
    void connectSignals();
    void applyConfig();
    void updateWindowTitle();
    void updateUndoRedoState();
    void updateZoomStatus();
    void updateBindingStatus();
    void updateFileStatus();

    // File operations
    void openDirectory();
    void onFileSelected(const QString &path);
    bool saveFile();
    bool saveFileAs();
    bool maybeSave();

    // Zoom operations
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();

    // Playback
    void onPlayPause();
    void onStop();
    void updatePlaybackState();

    // Scrollbar
    void updateScrollBar();
    void onScrollBarValueChanged(int value);

    // Playback range
    void playBoundarySegment(double timeSec);

signals:
    void documentLoaded(const QString &path);
    void documentSaved(const QString &path);
};

} // namespace phonemelabeler
} // namespace dstools
