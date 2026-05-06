#include "PhonemeTextGridTierLabel.h"

#include "IBoundaryModel.h"

#include <dstools/TimePos.h>

#include <QPainter>
#include <QPaintEvent>

namespace dstools {

PhonemeTextGridTierLabel::PhonemeTextGridTierLabel(QWidget *parent)
    : TierLabelArea(parent) {
    setFixedHeight(kTierRowHeight);
}

void PhonemeTextGridTierLabel::setBoundaryModel(IBoundaryModel *model) {
    TierLabelArea::setBoundaryModel(model);
    update();
}

int PhonemeTextGridTierLabel::activeTierIndex() const {
    return m_activeTierIndex;
}

void PhonemeTextGridTierLabel::setActiveTierIndex(int index) {
    if (m_activeTierIndex != index) {
        m_activeTierIndex = index;
        emit activeTierChanged(index);
        update();
    }
}

QList<double> PhonemeTextGridTierLabel::activeBoundaries() const {
    return TierLabelArea::activeBoundaries();
}

QList<QList<double>> PhonemeTextGridTierLabel::allTierBoundaries() const {
    QList<QList<double>> result;
    if (!m_boundaryModel)
        return result;

    int count = m_boundaryModel->tierCount();
    result.reserve(count);
    for (int t = 0; t < count; ++t) {
        QList<double> tierBounds;
        int bCount = m_boundaryModel->boundaryCount(t);
        tierBounds.reserve(bCount);
        for (int b = 0; b < bCount; ++b)
            tierBounds.append(usToSec(m_boundaryModel->boundaryTime(t, b)));
        result.append(tierBounds);
    }
    return result;
}

int PhonemeTextGridTierLabel::tierCount() const {
    if (!m_boundaryModel)
        return 0;
    return m_boundaryModel->tierCount();
}

int PhonemeTextGridTierLabel::tierRowHeight() const {
    return kTierRowHeight;
}

void PhonemeTextGridTierLabel::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    int w = width();
    int h = height();

    painter.fillRect(rect(), QColor(40, 40, 45));

    if (!m_boundaryModel || m_boundaryModel->tierCount() == 0) {
        painter.setPen(QColor(128, 128, 128));
        painter.drawText(rect(), Qt::AlignCenter, tr("No label data"));
        if (m_alignmentRunning) {
            painter.setPen(QColor(100, 180, 255));
            painter.drawText(rect().adjusted(0, 2, -4, 0),
                             Qt::AlignTop | Qt::AlignRight,
                             tr("Aligning..."));
        }
        return;
    }

    // Draw boundary lines for all tiers in a single row
    int tiers = m_boundaryModel->tierCount();
    auto allBounds = allTierBoundaries();

    for (int t = 0; t < tiers; ++t) {
        bool isActive = (t == m_activeTierIndex);
        const auto &bounds = allBounds[t];

        for (double bTime : bounds) {
            int x = timeToX(bTime);
            if (x < 0 || x > w)
                continue;

            if (isActive) {
                painter.setPen(QPen(QColor(255, 200, 100), 1));
            } else {
                painter.setPen(QPen(QColor(100, 100, 120), 1, Qt::DashLine));
            }
            painter.drawLine(x, 0, x, h);
        }
    }

    // Bottom border
    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, h - 1, w, h - 1);

    if (m_alignmentRunning) {
        painter.setPen(QColor(100, 180, 255));
        painter.drawText(rect().adjusted(0, 2, -4, 0),
                         Qt::AlignTop | Qt::AlignRight,
                         tr("Aligning..."));
    }
}

void PhonemeTextGridTierLabel::onModelDataChanged() {
    update();
}

void PhonemeTextGridTierLabel::setAlignmentRunning(bool running) {
    if (m_alignmentRunning != running) {
        m_alignmentRunning = running;
        update();
    }
}

} // namespace dstools
