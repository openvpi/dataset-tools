/// @file BoundaryOverlayWidget.h
/// @brief Transparent boundary line overlay drawn across multiple visualization widgets.

#pragma once

#include <QWidget>

#include <dstools/ViewportController.h>

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class TextGridDocument;

/// @brief Draws vertical boundary lines from the active tier across waveform,
///        spectrogram, and power widgets.
class BoundaryOverlayWidget : public QWidget {
    Q_OBJECT

public:
    /// @brief Constructs the overlay widget.
    /// @param viewport Viewport controller for coordinate synchronization.
    /// @param parent Optional parent widget.
    explicit BoundaryOverlayWidget(ViewportController *viewport, QWidget *parent = nullptr);

    /// @brief Sets the TextGrid document whose boundaries are drawn.
    /// @param doc TextGrid document.
    void setDocument(TextGridDocument *doc);

    /// @brief Tracks a widget to overlay on top of (typically the splitter).
    /// @param widget Widget to track for geometry changes.
    void trackWidget(QWidget *widget);

public slots:
    /// @brief Updates the viewport range for coordinate mapping.
    /// @param state New viewport state.
    void setViewport(const ViewportState &state);

    /// @brief Sets the index of the currently hovered boundary.
    /// @param index Boundary index, or -1 for none.
    void setHoveredBoundary(int index);

    /// @brief Sets the index of the currently dragged boundary.
    /// @param index Boundary index, or -1 for none.
    void setDraggedBoundary(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /// @brief Converts a time value to a pixel x-coordinate.
    [[nodiscard]] int timeToX(double time) const;

    /// @brief Repositions this overlay to match the tracked widget's geometry.
    void repositionOverSplitter();

    ViewportController *m_viewport = nullptr;   ///< Viewport controller.
    TextGridDocument *m_document = nullptr;      ///< Associated document.
    QWidget *m_trackedWidget = nullptr;          ///< Widget this overlay covers.

    double m_viewStart = 0.0;                    ///< Visible time range start in seconds.
    double m_viewEnd = 10.0;                     ///< Visible time range end in seconds.

    int m_hoveredBoundary = -1;                  ///< Index of hovered boundary, or -1.
    int m_draggedBoundary = -1;                  ///< Index of dragged boundary, or -1.
};

} // namespace phonemelabeler
} // namespace dstools
