#pragma once

#include <QWidget>
#include <QSplitter>
#include <QScrollBar>
#include <QUndoStack>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QToolBar>

#include <dsfw/AppSettings.h>
#include <dstools/PlayWidget.h>
#include <dstools/ShortcutManager.h>

#include "../PhonemeLabelerKeys.h"
#include "ui/TextGridDocument.h"
#include <dstools/ViewportController.h>
#include "ui/BoundaryBindingManager.h"
#include "ui/WaveformWidget.h"
#include "ui/TierEditWidget.h"
#include "ui/PowerWidget.h"
#include "ui/SpectrogramWidget.h"
#include "ui/BoundaryOverlayWidget.h"
#include "ui/FileListPanel.h"
#include "ui/EntryListPanel.h"
#include "ui/TimeRulerWidget.h"

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class PhonemeLabelerPage : public QWidget {
    Q_OBJECT

public:
    explicit PhonemeLabelerPage(QWidget *parent = nullptr);
    ~PhonemeLabelerPage() override;

    void setWorkingDirectory(const QString &dir);
    [[nodiscard]] QString workingDirectory() const { return m_currentDir; }

    void openFile(const QString &path);

    [[nodiscard]] QList<QAction *> editActions() const;
    [[nodiscard]] QList<QAction *> viewActions() const;
    [[nodiscard]] QMenu *spectrogramColorMenu() const { return m_spectrogramColorMenu; }
    [[nodiscard]] dstools::widgets::ShortcutManager *shortcutManager() const { return m_shortcutManager; }
    [[nodiscard]] dstools::AppSettings &settings() { return m_settings; }

    [[nodiscard]] QUndoStack *undoStack() const { return m_undoStack; }
    [[nodiscard]] QString currentFilePath() const { return m_currentFilePath; }
    [[nodiscard]] bool hasUnsavedChanges() const;
    bool save();
    bool saveAs();
    bool maybeSave();

    [[nodiscard]] QToolBar *toolbar() const { return m_toolbar; }

signals:
    void workingDirectoryChanged(const QString &dir);
    void fileSelected(const QString &path);
    void modificationChanged(bool modified);
    void positionChanged(double sec);
    void zoomChanged(double pixelsPerSecond);
    void bindingChanged(bool enabled);
    void fileStatusChanged(const QString &fileName);
    void documentLoaded(const QString &path);
    void documentSaved(const QString &path);

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
    QToolBar *m_toolbar = nullptr;

    // Actions
    QAction *m_actSave = nullptr;
    QAction *m_actSaveAs = nullptr;
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

    // Toolbar actions
    QAction *m_actPlayPause = nullptr;
    QAction *m_actStop = nullptr;
    QAction *m_actBindingToggle = nullptr;

    // Helpers
    void buildLayout();
    void buildActions();
    void buildToolbar();
    void connectSignals();
    void applyConfig();
    void updateAllBoundaryOverlays();

    // File operations
    void onFileSelected(const QString &path);
    bool saveFile();
    bool saveFileAs();

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
};

} // namespace phonemelabeler
} // namespace dstools
