#pragma once

/// @file MiniMapScrollBar.h
/// @brief Miniature waveform overview with a draggable viewport window.
///
/// Displays the full audio peak envelope and a semi-transparent rectangle
/// representing the current visible range.  Dragging the rectangle scrolls
/// the viewport; dragging its left/right edges zooms.

#include <QWidget>
#include <vector>

#include <dstools/ViewportController.h>

namespace dstools {

class MiniMapScrollBar : public QWidget {
    Q_OBJECT

public:
    explicit MiniMapScrollBar(widgets::ViewportController *viewport,
                              QWidget *parent = nullptr);
    ~MiniMapScrollBar() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void setViewport(const dsfw::widgets::ViewportState &state);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildPeakCache();
    [[nodiscard]] double xToTime(int x) const;
    [[nodiscard]] int timeToX(double sec) const;

    enum class DragMode {
        None,
        Scroll,
        ZoomLeft,
        ZoomRight,
    };

    widgets::ViewportController *m_viewport = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    struct PeakPair {
        float min;
        float max;
    };
    std::vector<PeakPair> m_peakCache;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_totalDuration = 0.0;

    DragMode m_dragMode = DragMode::None;
    int m_dragStartX = 0;
    double m_dragStartViewStart = 0.0;
    double m_dragStartViewEnd = 10.0;

    static constexpr int kFixedHeight = 30;
    static constexpr int kEdgeWidth = 6;
};

} // namespace dstools
