#include "PhonemeTextGridTierLabel.h"

#include "IBoundaryModel.h"

#include <dstools/TimePos.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>

namespace dstools {

PhonemeTextGridTierLabel::PhonemeTextGridTierLabel(QWidget *parent)
    : TierLabelArea(parent) {
    setFixedHeight(kTierRowHeight);
}

void PhonemeTextGridTierLabel::setBoundaryModel(IBoundaryModel *model) {
    TierLabelArea::setBoundaryModel(model);
    updateHeight();
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

void PhonemeTextGridTierLabel::updateHeight() {
    int tiers = tierCount();
    setFixedHeight(tiers > 0 ? kTierRowHeight * tiers : kTierRowHeight);
}

void PhonemeTextGridTierLabel::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    int w = width();

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

    int tiers = m_boundaryModel->tierCount();
    auto allBounds = allTierBoundaries();

    for (int t = 0; t < tiers; ++t) {
        int rowY = t * kTierRowHeight;
        bool isActive = (t == m_activeTierIndex);

        if (isActive) {
            painter.fillRect(0, rowY, w, kTierRowHeight, QColor(55, 55, 65));
        }

        QString name = m_boundaryModel->tierName(t);
        if (!name.isEmpty()) {
            painter.setPen(isActive ? QColor(255, 200, 100) : QColor(160, 160, 170));
            painter.drawText(QRect(2, rowY, kLabelWidth - 4, kTierRowHeight),
                             Qt::AlignVCenter | Qt::AlignLeft, name);
        }

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
            painter.drawLine(x, rowY, x, rowY + kTierRowHeight);
        }

        if (t < tiers - 1) {
            painter.setPen(QPen(QColor(60, 60, 70), 1));
            painter.drawLine(0, rowY + kTierRowHeight - 1, w, rowY + kTierRowHeight - 1);
        }
    }

    if (m_alignmentRunning) {
        painter.setPen(QColor(100, 180, 255));
        painter.drawText(rect().adjusted(0, 2, -4, 0),
                         Qt::AlignTop | Qt::AlignRight,
                         tr("Aligning..."));
    }
}

void PhonemeTextGridTierLabel::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_boundaryModel) {
        int tierIdx = event->position().y() / kTierRowHeight;
        if (tierIdx >= 0 && tierIdx < tierCount()) {
            setActiveTierIndex(tierIdx);
        }
    }
    QWidget::mousePressEvent(event);
}

void PhonemeTextGridTierLabel::onModelDataChanged() {
    updateHeight();
    update();
}

void PhonemeTextGridTierLabel::setAlignmentRunning(bool running) {
    if (m_alignmentRunning != running) {
        m_alignmentRunning = running;
        update();
    }
}

} // namespace dstools
