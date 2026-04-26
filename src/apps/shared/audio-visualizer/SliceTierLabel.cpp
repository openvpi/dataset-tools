#include "SliceTierLabel.h"

#include <dsfw/Theme.h>

#include <QPainter>
#include <QPaintEvent>

namespace dstools {

SliceTierLabel::SliceTierLabel(QWidget *parent)
    : TierLabelArea(parent) {
    setFixedHeight(28);
    connect(&dsfw::Theme::instance(), &dsfw::Theme::themeChanged,
            this, QOverload<>::of(&QWidget::update));
}

int SliceTierLabel::activeTierIndex() const {
    return 0;
}

void SliceTierLabel::setActiveTierIndex(int /*index*/) {
}

void SliceTierLabel::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    const auto &pal = dsfw::Theme::instance().palette();

    painter.fillRect(rect(), pal.phonemeEditor.tierBackgroundInactive);

    auto boundaries = activeBoundaries();
    if (boundaries.isEmpty()) {
        painter.setPen(pal.textSecondary);
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

        int x1 = static_cast<int>(timeToX(segStart));
        int x2 = static_cast<int>(timeToX(segEnd));

        if (x2 < 0 || x1 > width())
            continue;

        painter.setPen(QPen(pal.phonemeEditor.boundaryDragged, 1));
        painter.drawLine(x1, 0, x1, height());

        int segWidth = x2 - x1;
        if (segWidth > 20) {
            painter.setPen(pal.phonemeEditor.slicerLabelText);
            QString label = QString("%1").arg(i + 1, 3, 10, QChar('0'));
            QRect textRect(x1 + 2, 0, segWidth - 4, height());
            painter.drawText(textRect, Qt::AlignCenter, label);
        }
    }

    if (!boundaries.isEmpty()) {
        int x = static_cast<int>(timeToX(boundaries.last()));
        painter.setPen(QPen(pal.phonemeEditor.boundaryDragged, 1));
        painter.drawLine(x, 0, x, height());
    }

    painter.setPen(QPen(pal.phonemeEditor.slicerBorderLine, 1));
    painter.drawLine(0, 0, width(), 0);
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

} // namespace dstools
