#include "PlayCursorOverlay.h"

#include <QPaintEvent>
#include <QPainter>
#include <algorithm>
#include <dsfw/Theme.h>

namespace dstools {

    PlayCursorOverlay::PlayCursorOverlay(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        setStyleSheet("background: transparent;");
        connect(&dsfw::Theme::instance(), &dsfw::Theme::themeChanged, this, QOverload<>::of(&QWidget::update));
    }

    void PlayCursorOverlay::setPlayheadPosition(double positionSec) {
        if (m_positionSec == positionSec)
            return;
        m_positionSec = positionSec;
        update();
    }

    void PlayCursorOverlay::setViewStart(double startSec) {
        m_viewStart = startSec;
    }

    void PlayCursorOverlay::setViewEnd(double endSec) {
        m_viewEnd = endSec;
    }

    double PlayCursorOverlay::timeToX(double timeSec) const {
        if (m_converter)
            return m_converter->timeToX(timeSec, width());
        return 0.0;
    }

    void PlayCursorOverlay::paintEvent(QPaintEvent *) {
        if (m_positionSec < 0.0)
            return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        double x = std::clamp(timeToX(m_positionSec), 0.0, static_cast<double>(width()));

        painter.setPen(QPen(dsfw::Theme::instance().palette().pianoRoll.playhead, 2));
        painter.drawLine(QPointF(x, 0), QPointF(x, height()));
    }

} // namespace dstools
