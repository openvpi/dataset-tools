#pragma once

#include "NoteBoundaryModel.h"
#include "PianoRollInputHandler.h"
#include "PianoRollRenderer.h"
#include "PitchProcessor.h"

#include <ChartCoordinate.h>
#include <QAction>
#include <QActionGroup>
#include <QFrame>
#include <QMenu>
#include <QScrollBar>
#include <QSet>
#include <QUndoStack>
#include <QVector>
#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>
#include <dsfw/widgets/ViewportController.h>
#include <map>
#include <memory>
#include <set>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    class AppSettings;

    namespace pitchlabeler {


        namespace ui {

            using dstools::freqToMidi;
            using dstools::midiToFreq;
            using dstools::midiToNoteName;
            using dstools::midiToNoteString;
            using dstools::NotePitch;
            using dstools::parseNoteName;
            using dstools::shiftNoteCents;

            /// Piano roll view for editing notes and F0
            class PianoRollView : public QFrame {
                Q_OBJECT

            public:
                explicit PianoRollView(QWidget *parent = nullptr);
                ~PianoRollView() override;

                void setDsPitchDocument(std::shared_ptr<DsPitchDocument> ds);
                void setUndoStack(QUndoStack *stack) {
                    m_undoStack = stack;
                }
                void clear();

                /// Set audio file duration (seconds) to limit piano roll max length
                void setAudioDuration(double sec);

                /// Set shared viewport controller for horizontal zoom/scroll synchronization
                void setViewportController(dsfw::widgets::ViewportController *vc);

                void setContentLeftMargin(int margin) {
                    m_contentLeftMargin = margin;
                    m_inputHandler.setContentLeftMargin(margin);
                }

                // Zoom controls
                void zoomIn();
                void zoomOut();
                void resetZoom();
                int getZoomPercent() const;

                // Playhead
                void setPlayheadTime(double sec);
                void setPlayheadState(bool playing);

                // Tool mode
                void setToolMode(ToolMode mode);
                ToolMode toolMode() const {
                    return m_toolMode;
                }

                // A/B comparison
                void setABComparisonActive(bool active);
                bool isABComparisonActive() const {
                    return m_abComparisonActive;
                }

                // Boundary model
                void setBoundaryModel(chart::IBoundaryModel *model);
                chart::IBoundaryModel *boundaryModel() const;

                // Config persistence (following SlurCutter F0Widget pattern)
                void loadConfig(dstools::AppSettings &settings);
                void pullConfig(dstools::AppSettings &settings) const;

            signals:
                void noteSelected(int noteIndex);
                void selectionChanged(const std::set<int> &selected);
                void positionClicked(double time, double midi);
                void rulerClicked(double timeSec);
                void fileEdited();
                void toolModeChanged(int mode);
                void noteGlideChanged(int noteIndex, const QString &glide);
                void noteSlurToggled(int noteIndex);
                void noteRestToggled(int noteIndex);
                void noteMergeLeft(int noteIndex);
                void noteDeleteRequested(const std::vector<int> &indices);
                void verticalScrolled();

            public:
                /// Split a note at all phoneme boundaries within its time range
                void splitNoteAtBoundary(int noteIndex);
                /// Split all notes that overlap phoneme boundaries
                void snapAllNotesToPhoneme();

                /// Build a RenderState snapshot for rendering (also used by parent ChartPanel)
                RenderState buildRenderState() const;

                /// Render the piano roll content using the given coordinate
                void render(QPainter &painter, const chart::ChartCoordinate &coord);

                /// Update the unified coordinate from viewport state
                void updateCoord();

            signals:
                /// Emitted when a note split is requested
                void noteSplitRequested(int noteIndex);
                /// Emitted when snap to phoneme is requested
                void snapToPhonemeRequested();

            protected:
                void paintEvent(QPaintEvent *event) override;
                void resizeEvent(QResizeEvent *event) override;
                void wheelEvent(QWheelEvent *event) override;
                void mousePressEvent(QMouseEvent *event) override;
                void mouseMoveEvent(QMouseEvent *event) override;
                void mouseReleaseEvent(QMouseEvent *event) override;
                void mouseDoubleClickEvent(QMouseEvent *event) override;
                void keyPressEvent(QKeyEvent *event) override;
                void contextMenuEvent(QContextMenuEvent *event) override;

            private:
                // Composed components
                PianoRollInputHandler m_inputHandler;

                // Viewport controller (shared horizontal zoom/scroll)
                dsfw::widgets::ViewportController *m_viewport = nullptr;
                void onViewportChanged(const dsfw::widgets::ViewportState &state);

                // Data
                std::shared_ptr<DsPitchDocument> m_dsFile;
                QUndoStack *m_undoStack = nullptr;
                double m_audioDuration = 0.0;

                // Display parameters (vertical scale only, horizontal uses ViewportController)
                double m_vScale = 20.0;

                int m_contentLeftMargin = RenderState::PianoWidth;

                // Unified coordinate (from ChartCoordinate)
                chart::ChartCoordinate m_coord;

                // Scroll bar (vertical only; horizontal scroll via ViewportController)
                QScrollBar *m_vScrollBar = nullptr;

                // Viewport offset
                double m_scrollY = 0.0;

                // Playhead
                double m_playheadPos = 0.0;
                bool m_isPlaying = false;

                // Tool mode
                ToolMode m_toolMode = ToolSelect;

                // A/B comparison
                bool m_abComparisonActive = false;
                std::vector<float> m_originalF0;

                // Display options
                bool m_showPitchTextOverlay = false;
                bool m_showPhonemeTexts = false; // 根据用户要求，pitchlabel不需要标签区域
                bool m_showCrosshairAndPitch = true;
                bool m_snapToKey = false;

                // Context note for right-click menu
                int m_contextNoteIndex = -1;

                // Multi-selection
                std::set<int> m_selectedNotes;

                // Boundary model for note start/end dragging
                std::unique_ptr<NoteBoundaryModel> m_boundaryModel;

                /// External phoneme boundary model for split/merge operations
                chart::IBoundaryModel *m_phonemeBoundaryModel = nullptr;
                int m_draggingBoundaryIndex = -1;
                bool m_isDraggingBoundary = false;

                // Layout constants (defined in PianoRollRenderer::RenderState)

                // Coordinate conversion
                double timeToX(double time) const;
                double xToTime(double x) const;
                double midiToY(double midi) const;
                double yToMidi(double y) const;
                int sceneXToWidget(double sceneX) const;
                int sceneYToWidget(double sceneY) const;
                double widgetXToScene(int wx) const;
                double widgetYToScene(int wy) const;

                // Helper methods

                void updateScrollBars();

                // Note helpers
                int getNoteAtPosition(int x, int y) const;
                int getNoteBoundaryAtPosition(int x, int y) const;

                // Selection helpers
                void selectNotes(const std::set<int> &indices);
                void selectAllNotes();
                void clearSelection();

                // Pitch editing
                void doPitchMove(const std::vector<int> &indices, int deltaCents);

                // Build RenderState snapshot for renderer

                // Boundary dragging
                void startBoundaryDrag(int boundaryIndex);
                void updateBoundaryDrag(int boundaryIndex, int deltaPixels);
                void endBoundaryDrag();
                void cancelBoundaryDrag();

                // Setup input handler callbacks
                void setupInputCallbacks();

                // Restore cursor to tool-mode default
                void restoreToolCursor();

                // Context menus
                QMenu *m_bgMenu = nullptr;
                QMenu *m_noteMenu = nullptr;
                QAction *m_actShowPitchTextOverlay = nullptr;
                QAction *m_actShowPhonemeTexts = nullptr;
                QAction *m_actShowCrosshairAndPitch = nullptr;
                QAction *m_actSnapToKey = nullptr;

                QAction *m_actMergeLeft = nullptr;
                QAction *m_actToggleRest = nullptr;
                QAction *m_actToggleSlur = nullptr;
                QActionGroup *m_actGlideGroup = nullptr;
                QAction *m_actGlideNone = nullptr;
                QAction *m_actGlideUp = nullptr;
                QAction *m_actGlideDown = nullptr;

                void buildContextMenu();
                void buildNoteMenu();
                void updateNoteMenuState();
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools
