/// @file TierEditWidget.h
/// @brief Container widget managing multiple IntervalTierView instances.

#pragma once

#include <QPoint>
#include <QUndoStack>
#include <QVector>
#include <QWidget>

#include "TextGridDocument.h"
#include <dstools/ViewportController.h>

class QVBoxLayout;

namespace dstools {
    namespace phonemelabeler {

        using dstools::widgets::ViewportController;
        using dstools::widgets::ViewportState;

        class BoundaryDragController;
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

        public slots:
            /// @brief Slot invoked when the viewport changes.
            /// @param state New viewport state.
            void onViewportChanged(const ViewportState &state);
            void rebuildTierViews(); ///< Recreates IntervalTierView widgets for all tiers.

        signals:
            /// @brief Emitted when a tier is activated by user interaction.
            /// @param tierIndex Index of the activated tier.
            void tierActivated(int tierIndex);

        protected:
            void resizeEvent(QResizeEvent *event) override;

        private:
            void updateViewport(); ///< Propagates viewport state to all tier views.

            TextGridDocument *m_document = nullptr;             ///< Associated document.
            QUndoStack *m_undoStack = nullptr;                  ///< Undo stack.
            ViewportController *m_viewport = nullptr;           ///< Viewport controller.
            BoundaryDragController *m_dragController = nullptr; ///< Drag controller.

            QVBoxLayout *m_layout = nullptr;         ///< Layout for tier views.
            QVector<IntervalTierView *> m_tierViews; ///< List of tier view widgets.

            double m_viewStart = 0.0;         ///< Visible range start in seconds.
            double m_viewEnd = 10.0;          ///< Visible range end in seconds.
        };

    } // namespace phonemelabeler
} // namespace dstools
