/// @file PhonemeEditor.h
/// @brief Reusable TextGrid phoneme editor widget extracted from PhonemeLabelerPage.

#pragma once

#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QScrollBar>
#include <QSplitter>
#include <QToolBar>
#include <QUndoStack>
#include <QWidget>

#include <dstools/PlayWidget.h>
#include <dstools/ViewportController.h>
#include <dsfw/widgets/TimeRulerWidget.h>

#include "ui/BoundaryBindingManager.h"
#include "ui/BoundaryOverlayWidget.h"
#include "ui/EntryListPanel.h"
#include "ui/PowerWidget.h"
#include "ui/SpectrogramWidget.h"
#include "ui/TextGridDocument.h"
#include "ui/TierEditWidget.h"
#include "ui/WaveformRenderer.h"
#include "ui/WaveformWidget.h"

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;
using dsfw::widgets::TimeRulerWidget;

/// @brief Core phoneme editing widget: waveform/spectrogram/power visualization,
/// multi-tier TextGrid editing with undo/redo, boundary binding, and playback.
///
/// Contains no file I/O, no file list panel, no IPageActions, no settings
/// persistence. PhonemeLabelerPage (and future DsPhonemeLabelerPage) compose
/// this widget and connect it to their own data sources.
class PhonemeEditor : public QWidget {
    Q_OBJECT

public:
    explicit PhonemeEditor(QWidget *parent = nullptr);
    ~PhonemeEditor() override = default;

    /// Load a TextGrid document into the editor.
    void setDocument(TextGridDocument *doc);

    /// Load audio for waveform/spectrogram/power rendering and playback.
    void loadAudio(const QString &audioPath);

    /// Access the underlying document.
    [[nodiscard]] TextGridDocument *document() const { return m_document; }

    /// Access the undo stack.
    [[nodiscard]] QUndoStack *undoStack() const { return m_undoStack; }

    /// Access the viewport controller.
    [[nodiscard]] ViewportController *viewport() const { return m_viewport; }

    /// Access the play widget for playback control.
    [[nodiscard]] dstools::widgets::PlayWidget *playWidget() const { return m_playWidget; }

    /// Access the toolbar (for embedding in parent layouts if needed).
    [[nodiscard]] QToolBar *toolbar() const { return m_toolbar; }

    /// Access the boundary binding manager.
    [[nodiscard]] BoundaryBindingManager *bindingManager() const { return m_bindingManager; }

    /// Access the entry list panel.
    [[nodiscard]] EntryListPanel *entryListPanel() const { return m_entryListPanel; }

    /// Access the spectrogram color menu (for embedding in parent menus).
    [[nodiscard]] QMenu *spectrogramColorMenu() const { return m_spectrogramColorMenu; }

    // --- View actions (for menu/shortcut binding by the page) ---
    [[nodiscard]] QAction *zoomInAction() const { return m_actZoomIn; }
    [[nodiscard]] QAction *zoomOutAction() const { return m_actZoomOut; }
    [[nodiscard]] QAction *zoomResetAction() const { return m_actZoomReset; }
    [[nodiscard]] QAction *toggleBindingAction() const { return m_actToggleBinding; }
    [[nodiscard]] QAction *togglePowerAction() const { return m_actTogglePower; }
    [[nodiscard]] QAction *toggleSpectrogramAction() const { return m_actToggleSpectrogram; }
    [[nodiscard]] QAction *saveAction() const { return m_actSave; }
    [[nodiscard]] QAction *saveAsAction() const { return m_actSaveAs; }
    [[nodiscard]] QAction *undoAction() const { return m_actUndo; }
    [[nodiscard]] QAction *redoAction() const { return m_actRedo; }
    [[nodiscard]] QAction *playPauseAction() const { return m_actPlayPause; }
    [[nodiscard]] QAction *stopAction() const { return m_actStop; }
    [[nodiscard]] QList<QAction *> viewActions() const;

    // --- Configuration ---
    void setBindingEnabled(bool enabled);
    void setBindingToleranceMs(double ms);
    void setPixelsPerSecond(double pps);
    void setPowerVisible(bool visible);
    void setSpectrogramVisible(bool visible);
    void setSpectrogramColorStyle(const QString &styleName);

    QByteArray saveSplitterState() const;
    void restoreSplitterState(const QByteArray &state);

signals:
    void modificationChanged(bool modified);
    void positionChanged(double sec);
    void zoomChanged(double pixelsPerSecond);
    void bindingChanged(bool enabled);
    void fileStatusChanged(const QString &fileName);
    void documentLoaded();
    void documentSaved();

private:
    // Document
    TextGridDocument *m_document = nullptr;
    QUndoStack *m_undoStack = nullptr;
    ViewportController *m_viewport = nullptr;
    BoundaryBindingManager *m_bindingManager = nullptr;

    // Services
    dstools::widgets::PlayWidget *m_playWidget = nullptr;
    WaveformRenderer *m_renderer = nullptr;

    // UI Components
    QSplitter *m_mainSplitter = nullptr;
    QSplitter *m_rightSplitter = nullptr;
    WaveformWidget *m_waveformWidget = nullptr;
    TierEditWidget *m_tierEditWidget = nullptr;
    PowerWidget *m_powerWidget = nullptr;
    SpectrogramWidget *m_spectrogramWidget = nullptr;
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
    void buildActions();
    void buildToolbar();
    void buildLayout();
    void connectSignals();
    void updateAllBoundaryOverlays();

    // Zoom
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
