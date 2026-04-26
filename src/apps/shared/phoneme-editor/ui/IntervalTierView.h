/// @file IntervalTierView.h
/// @brief Interactive interval tier visualization and editing widget.

#pragma once

#include "ChartCoordinate.h"
#include "TextGridDocument.h"

#include <QPoint>
#include <QVector>
#include <QWidget>
#include <dstools/TimePos.h>
#include <dsfw/widgets/ViewportController.h>

class QPainter;
class QUndoStack;

namespace dstools {
    namespace chart {
        class BoundaryDragController;
    }
    namespace phonemelabeler {

        using dstools::chart::ChartCoordinate;
        using dstools::chart::BoundaryDragController;
        using dsfw::widgets::ViewportController;
        using dsfw::widgets::ViewportState;

        /// @brief Renders a single TextGrid interval tier with boundary dragging,
        ///        interval selection, keyboard navigation, and binding-aware multi-tier editing.
        class IntervalTierView : public QWidget {
            Q_OBJECT

        public:
            /// @brief Constructs a view for one interval tier.
            /// @param tierIndex Index of the tier in the document.
            /// @param doc TextGrid document.
            /// @param undoStack Undo stack for edit commands.
            /// @param viewport Viewport controller for time-to-pixel mapping.
            /// @param bindingMgr Boundary binding manager for cross-tier linking.
            /// @param parent Optional parent widget.
            explicit IntervalTierView(int tierIndex, TextGridDocument *doc, QUndoStack *undoStack,
                                      ViewportController *viewport, BoundaryDragController *dragController,
                                      QWidget *parent = nullptr);
            void setViewport(const ViewportState &state);
            ~IntervalTierView() override = default;

            /// @brief Updates the viewport state for rendering.
            /// @param state New viewport state.
            void setViewStart(double startSec);
            void setViewEnd(double endSec);
            void setCoordConverter(const ChartCoordinate *conv) {
                m_converter = conv;
            }

            /// @brief Returns the tier index this view represents.
            int tierIndex() const {
                return m_tierIndex;
            }

            /// @brief Sets whether this tier view is the active (focused) tier.
            /// @param active True to mark as active.
            void setActive(bool active);

            /// @brief Returns whether this tier view is active.
            [[nodiscard]] bool isActive() const {
                return m_active;
            }

            /// @brief Selects the next interval (keyboard navigation).
            void selectNextInterval();

            /// @brief Selects the previous interval (keyboard navigation).
            void selectPrevInterval();

            /// @brief Adjusts the selected boundary position by a time delta.
            /// @param deltaSec Time adjustment in seconds.
            void adjustSelectedBoundary(TimePos deltaUs);

        signals:
            /// @brief Emitted when an interval is clicked.
            /// @param intervalIndex Index of the clicked interval.
            /// @param startTime Start time of the interval.
            /// @param endTime End time of the interval.
            void intervalClicked(int intervalIndex, TimePos startTime, TimePos endTime);

            /// @brief Emitted when an interval is double-clicked.
            /// @param intervalIndex Index of the double-clicked interval.
            void intervalDoubleClicked(int intervalIndex);

            /// @brief Emitted when a boundary drag begins.
            /// @param boundaryIndex Index of the dragged boundary.
            void boundaryDragStarted(int boundaryIndex);

            /// @brief Emitted to request playback of an interval.
            /// @param startTime Playback start time.
            /// @param endTime Playback end time.
            void requestPlayback(TimePos startTime, TimePos endTime);

            /// @brief Emitted when this tier is activated by user interaction.
            /// @param tierIndex Index of the activated tier.
            void activated(int tierIndex);

            /// @brief Emitted when a boundary is hovered in this tier.
            /// @param boundaryIndex Index of the hovered boundary, or -1 if none.
            void hoveredBoundaryChanged(int boundaryIndex);

        protected:
            void paintEvent(QPaintEvent *event) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void mouseDoubleClickEvent(QMouseEvent *event) override;
            void leaveEvent(QEvent *event) override;
            void keyPressEvent(QKeyEvent *event) override;

        private:
            [[nodiscard]] double timeToX(double time) const; ///< Converts time to pixel x.
            [[nodiscard]] double xToTime(int x) const;       ///< Converts pixel x to time.
            [[nodiscard]] int hitTestBoundary(int x) const;  ///< Returns boundary index at x, or -1.
            [[nodiscard]] int hitTestInterval(int x) const;  ///< Returns interval index at x, or -1.

            void drawIntervals(QPainter &painter);    ///< Draws interval backgrounds.
            void drawLabels(QPainter &painter);       ///< Draws interval labels.
            void drawBindingLines(QPainter &painter); ///< Draws cross-tier binding indicators.
            void drawSelection(QPainter &painter);    ///< Draws selection highlight.

            int m_tierIndex;                                    ///< Tier index in the document.
            TextGridDocument *m_doc = nullptr;                  ///< Associated document.
            QUndoStack *m_undoStack = nullptr;                  ///< Undo stack for commands.
            ViewportController *m_viewport = nullptr;           ///< Viewport controller.
            BoundaryDragController *m_dragController = nullptr; ///< Drag controller.

            double m_viewStart = 0.0;                     ///< Visible range start in seconds.
            double m_viewEnd = 10.0;                      ///< Visible range end in seconds.
            const ChartCoordinate *m_converter = nullptr; ///< Shared coordinate converter.

            int m_hoveredBoundary = -1;  ///< Hovered boundary index, or -1.
            int m_selectedInterval = -1; ///< Selected interval for keyboard nav.

            static constexpr int kBoundaryHitWidth = 12; ///< Hit-test width in pixels.
            bool m_active = false;                       ///< Whether this tier is active.
        };

    } // namespace phonemelabeler
} // namespace dstools
