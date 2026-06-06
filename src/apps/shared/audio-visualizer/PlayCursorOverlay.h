#pragma once

/// @file PlayCursorOverlay.h
/// @brief Transparent overlay that draws a red vertical playhead line.
///
/// Set WA_TransparentForMouseEvents so it does not intercept clicks.
/// Set WA_TranslucentBackground so the parent shows through.
/// The line spans the full widget height.

#include <ChartCoordinate.h>
#include <QWidget>

namespace dstools {

    class PlayCursorOverlay : public QWidget {
        Q_OBJECT
    public:
        explicit PlayCursorOverlay(QWidget *parent = nullptr);

        void setPlayheadPosition(double positionSec);
        void setViewStart(double startSec);
        void setViewEnd(double endSec);
        void setCoordConverter(const dstools::ChartCoordinate *conv) {
            m_converter = conv;
        }

    protected:
        void paintEvent(QPaintEvent *event) override;

    private:
        double timeToX(double timeSec) const;

        double m_positionSec = -1.0;
        double m_viewStart = 0.0;
        double m_viewEnd = 10.0;

        const dstools::ChartCoordinate *m_converter = nullptr;
    };

} // namespace dstools