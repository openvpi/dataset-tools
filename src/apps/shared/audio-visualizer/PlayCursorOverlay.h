#pragma once

/// @file PlayCursorOverlay.h
/// @brief Transparent overlay that draws a red vertical playhead line.
///
/// Set WA_TransparentForMouseEvents so it does not intercept clicks.
/// Set WA_TranslucentBackground so the parent shows through.
/// The line spans the full widget height.

#include <QWidget>
#include <QColor>

namespace dstools {

class PlayCursorOverlay : public QWidget {
    Q_OBJECT
public:
    explicit PlayCursorOverlay(QWidget *parent = nullptr);

    void setPlayheadPosition(double positionSec);
    void setViewStart(double startSec);
    void setViewEnd(double endSec);
    void setPixelWidth(int pixelWidth);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double timeToX(double timeSec) const;

    double m_positionSec = -1.0;
    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    int m_pixelWidth = 1200;

    QColor m_color{255, 60, 60, 220};
    int m_lineWidth = 2;
};

} // namespace dstools