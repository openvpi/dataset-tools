/// @file PhonemeEditor.h
/// @brief Reusable TextGrid phoneme editor widget extracted from PhonemeLabelerPage.

#pragma once

#include "EditorContainerBase.h"
#include "PianoRollChartPanel.h"
#include "PowerChartPanel.h"
#include "SpectrogramChartPanel.h"
#include "WaveformChartPanel.h"
#include "ui/EntryListPanel.h"
#include "ui/TextGridDocument.h"
#include "ui/TierEditWidget.h"
#include "ui/WaveformRenderer.h"

#include <BoundaryDragController.h>
#include <BoundaryOverlayWidget.h>
#include <dstools/DsPitchDocument.h>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QSplitter>
#include <QToolBar>
#include <QUndoStack>
#include <QWidget>
#include <dsfw/widgets/PlayWidget.h>
#include <dsfw/widgets/ViewportController.h>

namespace dstools {

    namespace phonemelabeler {

        using ::dstools::BoundaryDragController;
        using dsfw::widgets::ViewportController;
        using dsfw::widgets::ViewportState;

        /// @brief Core phoneme editing widget: waveform/spectrogram/power visualization,
        /// multi-tier TextGrid editing with undo/redo, boundary binding, and playback.
        ///
        /// Contains no file I/O, no file list panel, no IPageActions, no settings
        /// persistence. PhonemeLabelerPage (and future DsPhonemeLabelerPage) compose
        /// this widget and connect it to their own data sources.
        class PhonemeEditor : public EditorContainerBase {
            Q_OBJECT

        public:
            explicit PhonemeEditor(QWidget *parent = nullptr);
            ~PhonemeEditor() override = default;

            /// Load a TextGrid document into the editor.
            void setDocument(TextGridDocument *doc);

            /// Load audio for waveform/spectrogram/power rendering and playback.
            void loadAudio(const QString &audioPath);

            /// Load DsPitchDocument data into the piano roll view for pitch editing.
            void loadDsPitchDocument(std::shared_ptr<pitchlabeler::DsPitchDocument> dsFile);

            /// Get the piano roll view for direct pitch manipulation.
            [[nodiscard]] pitchlabeler::ui::PianoRollView *pianoRollView() const;

            /// Access the underlying document.
            [[nodiscard]] TextGridDocument *document() const {
                return m_document;
            }

            /// Access the viewport controller.
            [[nodiscard]] ViewportController *viewport() const {
                return EditorContainerBase::viewport();
            }

            /// Access the play widget for playback control.
            [[nodiscard]] dsfw::widgets::PlayWidget *playWidget() const {
                return EditorContainerBase::playWidget();
            }

            /// Access the toolbar (for embedding in parent layouts if needed).
            [[nodiscard]] QToolBar *toolbar() const {
                return m_toolbar;
            }

            /// Access the boundary drag controller.
            [[nodiscard]] BoundaryDragController *dragController() const;

            /// Access the entry list panel.
            [[nodiscard]] EntryListPanel *entryListPanel() const {
                return m_entryListPanel;
            }

            /// Access the piano roll chart panel.
            [[nodiscard]] pitchlabeler::ui::PianoRollChartPanel *pianoRollChart() const {
                return m_pianoRollChart;
            }

            /// Access the spectrogram color menu (for embedding in parent menus).
            [[nodiscard]] QMenu *spectrogramColorMenu() const {
                return m_spectrogramColorMenu;
            }

            // --- View actions (for menu/shortcut binding by the page) ---
            [[nodiscard]] QAction *zoomInAction() const {
                return m_actZoomIn;
            }
            [[nodiscard]] QAction *zoomOutAction() const {
                return m_actZoomOut;
            }
            [[nodiscard]] QAction *zoomResetAction() const {
                return m_actZoomReset;
            }
            [[nodiscard]] QAction *toggleBindingAction() const {
                return m_actToggleBinding;
            }
            [[nodiscard]] QAction *togglePowerAction() const {
                return m_actTogglePower;
            }
            [[nodiscard]] QAction *toggleSpectrogramAction() const {
                return m_actToggleSpectrogram;
            }
            [[nodiscard]] QAction *togglePianoRollAction() const {
                return m_actTogglePianoRoll;
            }
            [[nodiscard]] QAction *saveAction() const {
                return m_actSave;
            }
            [[nodiscard]] QAction *saveAsAction() const {
                return m_actSaveAs;
            }
            [[nodiscard]] QList<QAction *> viewActions() const;

            // --- Configuration ---
            void setBindingEnabled(bool enabled);
            void setSnapEnabled(bool enabled);
            void setBindingToleranceMs(double ms);
            void setPowerVisible(bool visible);
            void setSpectrogramVisible(bool visible);
            void setPianoRollVisible(bool visible);
            void setSpectrogramColorStyle(const QString &styleName);

            [[nodiscard]] bool isSnapEnabled() const {
                return m_snapEnabled;
            }
            /// Snap threshold in pixels. Boundaries within this distance snap on release.
            static constexpr int kSnapThresholdPx = 5;

            /// Save/restore chart visibility to AppSettings.
            void saveChartVisibility();
            void restoreChartVisibility();

        signals:
            void positionChanged(double sec);
            void bindingChanged(bool enabled);
            void fileStatusChanged(const QString &fileName);
            void documentLoaded();
            void documentSaved();

            /// Emitted when the power widget's visibility changes externally.
            void powerVisibilityChanged(bool visible);
            /// Emitted when the spectrogram widget's visibility changes externally.
            void spectrogramVisibilityChanged(bool visible);

            /// Emitted when the piano roll widget's visibility changes externally.
            void pianoRollVisibilityChanged(bool visible);

        private:
            // Document
            TextGridDocument *m_document = nullptr;

            // Services
            WaveformRenderer *m_renderer = nullptr;

            // UI Components
            TierEditWidget *m_tierEditWidget = nullptr;
            EntryListPanel *m_entryListPanel = nullptr;
            QToolBar *m_toolbar = nullptr;
            pitchlabeler::ui::PianoRollChartPanel *m_pianoRollChart = nullptr;

            // Actions
            QAction *m_actSave = nullptr;
            QAction *m_actSaveAs = nullptr;
            QAction *m_actZoomIn = nullptr;
            QAction *m_actZoomOut = nullptr;
            QAction *m_actZoomReset = nullptr;
            QAction *m_actToggleBinding = nullptr;
            QAction *m_actTogglePower = nullptr;
            QAction *m_actToggleSpectrogram = nullptr;
            QAction *m_actTogglePianoRoll = nullptr;
            QMenu *m_spectrogramColorMenu = nullptr;
            QActionGroup *m_spectrogramColorGroup = nullptr;

            // Toolbar actions
            QAction *m_actBindingToggle = nullptr;
            QAction *m_actSnapToggle = nullptr;
            bool m_snapEnabled = true;

            // Helpers
            void buildActions();
            void buildToolbar();
            void buildLayout();
            void connectSignals();
            void updateAllBoundaryOverlays();
            void updateBoundaryOverlayExclusions();

            // Playback range
            void playBoundarySegment(double timeSec);
        };

    } // namespace phonemelabeler
} // namespace dstools
