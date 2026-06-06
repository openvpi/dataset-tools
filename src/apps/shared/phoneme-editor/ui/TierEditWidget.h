/// @file TierEditWidget.h
/// @brief Container widget managing multiple IntervalTierView instances.

#pragma once

#include "ChartCoordinate.h"
#include "TextGridDocument.h"

#include <QButtonGroup>
#include <QPoint>
#include <QRadioButton>
#include <QUndoStack>
#include <QVector>
#include <QWidget>
#include <dsfw/widgets/ViewportController.h>

class QVBoxLayout;

namespace dstools {
    namespace dstools {
        class BoundaryDragController;
    }
    namespace phonemelabeler {

        using dstools::dstools::ChartCoordinate;
        using dstools::dstools::BoundaryDragController;
        using dsfw::widgets::ViewportController;
        using dsfw::widgets::ViewportState;

        class IntervalTierView;

        /// @brief Creates and lays out IntervalTierView widgets for each tier in the document,
        ///        synchronizing viewport state.
        class TierEditWidget : public QWidget {
            Q_OBJECT

        public:
            /// @brief Constructs the tier edit widget.
            /// @param doc TextGrid document.
            /// @param undoStack Undo stack for edit commands.
            /// @param viewport Viewport controller for time synchronization.
            /// @param bindingMgr Boundary binding manager.
            /// @param parent Optional parent widget.
            TierEditWidget(TextGridDocument *doc, QUndoStack *undoStack, ViewportController *viewport,
                           BoundaryDragController *dragController, QWidget *parent = nullptr);
            ~TierEditWidget() override;

            /// @brief Sets the TextGrid document and rebuilds tier views.
            /// @param doc TextGrid document.
            void setDocument(TextGridDocument *doc);

            /// @brief Updates the viewport state for all tier views.
            /// @param state New viewport state.
            void setViewport(const ViewportState &state);
            void setCoordConverter(const ChartCoordinate *conv);
            void setYAxisWidth(int w); ///< 设置单选按钮区域宽度
            QWidget *radioButtonContainer() const { return m_radioButtonContainer; }

        public slots:
            /// @brief Slot invoked when the viewport changes.
            /// @param state New viewport state.
            void onViewportChanged(const ViewportState &state);
            void rebuildTierViews(); ///< Recreates IntervalTierView widgets for all tiers.

        signals:
            /// @brief Emitted when a tier is activated by user interaction.
            /// @param tierIndex Index of the activated tier.
            void tierActivated(int tierIndex);

            /// @brief Emitted when a boundary is hovered in a tier view.
            /// @param tierIndex Index of the tier.
            /// @param boundaryIndex Index of the hovered boundary, or -1 if none.
            void tierHoveredBoundaryChanged(int tierIndex, int boundaryIndex);

            /// @brief Emitted when a tier view requests audio playback of an interval.
            /// @param startTime Interval start time in microseconds.
            /// @param endTime Interval end time in microseconds.
            void requestPlayback(TimePos startTime, TimePos endTime);

        protected:
            void resizeEvent(QResizeEvent *event) override;

        private:
            void updateViewport();     ///< Propagates viewport state to all tier views.
            void updateRadioButtons(); ///< 更新单选按钮状态

            TextGridDocument *m_document = nullptr;             ///< Associated document.
            QUndoStack *m_undoStack = nullptr;                  ///< Undo stack.
            ViewportController *m_viewport = nullptr;           ///< Viewport controller.
            BoundaryDragController *m_dragController = nullptr; ///< Drag controller.

            QVBoxLayout *m_layout = nullptr;         ///< Layout for tier views.
            QVector<IntervalTierView *> m_tierViews; ///< List of tier view widgets.

            QWidget *m_radioButtonContainer = nullptr; ///< 单选按钮容器
            QWidget *m_tierViewsContainer = nullptr;   ///< 层级视图容器
            QButtonGroup *m_buttonGroup = nullptr;     ///< 单选按钮组
            QVector<QRadioButton *> m_radioButtons;    ///< 单选按钮列表

            double m_viewStart = 0.0;                     ///< Visible range start in seconds.
            double m_viewEnd = 10.0;                      ///< Visible range end in seconds.
            const ChartCoordinate *m_converter = nullptr; ///< Shared coordinate converter.
        };

    } // namespace phonemelabeler
} // namespace dstools
