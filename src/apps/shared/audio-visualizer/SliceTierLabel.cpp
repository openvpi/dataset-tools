#include "SliceTierLabel.h"

#include <QPainter>
#include <QPaintEvent>

namespace dstools {

SliceTierLabel::SliceTierLabel(QWidget *parent)
    : TierLabelArea(parent) {
    setFixedHeight(28);
}

int SliceTierLabel::activeTierIndex() const {
    return 0;
}

void SliceTierLabel::setActiveTierIndex(int /*index*/) {
}

void SliceTierLabel::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 40, 45));

    auto boundaries = activeBoundaries();
    if (boundaries.isEmpty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No slices"));
        return;
    }

    QFont font = painter.font();
    font.setPixelSize(12);
    font.setBold(true);
    painter.setFont(font);

    std::vector<double> segBounds;
    segBounds.push_back(0.0);
    for (double b : boundaries)
        segBounds.push_back(b);

    for (size_t i = 0; i + 1 < segBounds.size(); ++i) {
        double segStart = segBounds[i];
        double segEnd = segBounds[i + 1];

        int x1 = timeToX(segStart);
        int x2 = timeToX(segEnd);

        if (x2 < 0 || x1 > width())
            continue;

        painter.setPen(QPen(QColor(255, 200, 100), 1));
        if (i > 0)
            painter.drawLine(x1, 0, x1, height());

        int segWidth = x2 - x1;
        if (segWidth > 20) {
            painter.setPen(QColor(200, 200, 220));
            QString label = QString("%1").arg(i + 1, 3, 10, QChar('0'));
            QRect textRect(x1 + 2, 0, segWidth - 4, height());
            painter.drawText(textRect, Qt::AlignCenter, label);
        }
    }

    if (!boundaries.isEmpty()) {
        int x = timeToX(boundaries.last());
        painter.setPen(QPen(QColor(255, 200, 100), 1));
        painter.drawLine(x, 0, x, height());
    }

    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, 0, width(), 0);
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

} // namespace dstools
