#include "PhonemeTextGridTierLabel.h"

#include "IBoundaryModel.h"

#include <dstools/TimePos.h>

#include <QComboBox>
#include <QPainter>
#include <QPaintEvent>

namespace dstools {

PhonemeTextGridTierLabel::PhonemeTextGridTierLabel(QWidget *parent)
    : TierLabelArea(parent) {
    m_tierCombo = new QComboBox(this);
    m_tierCombo->setFixedHeight(kTierRowHeight - 2);
    m_tierCombo->setMinimumWidth(kComboWidth);
    m_tierCombo->setMaximumWidth(160);
    m_tierCombo->setToolTip(tr("选择活跃层级（该层的边界线贯穿全图）"));

    connect(m_tierCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index >= 0)
            setActiveTierIndex(index);
    });

    setFixedHeight(kTierRowHeight);
}

void PhonemeTextGridTierLabel::setBoundaryModel(IBoundaryModel *model) {
    TierLabelArea::setBoundaryModel(model);
    rebuildComboBox();
}

int PhonemeTextGridTierLabel::activeTierIndex() const {
    return m_activeTierIndex;
}

void PhonemeTextGridTierLabel::setActiveTierIndex(int index) {
    if (m_activeTierIndex != index) {
        m_activeTierIndex = index;
        if (m_tierCombo && index >= 0 && index < m_tierCombo->count())
            m_tierCombo->setCurrentIndex(index);
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
                             tr("对齐中..."));
        }
        return;
    }

    // Draw boundary lines in the label area for ALL tiers.
    // Active tier lines are solid; non-active are dashed.
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
                         tr("对齐中..."));
    }
}

void PhonemeTextGridTierLabel::resizeEvent(QResizeEvent *event) {
    TierLabelArea::resizeEvent(event);
    // Position combo box at the left edge, vertically centered
    if (m_tierCombo) {
        int comboW = qMin(m_tierCombo->sizeHint().width(), 160);
        int y = (height() - m_tierCombo->height()) / 2;
        m_tierCombo->setGeometry(2, qMax(y, 1), comboW, m_tierCombo->height());
    }
}

void PhonemeTextGridTierLabel::onModelDataChanged() {
    rebuildComboBox();
}

void PhonemeTextGridTierLabel::setAlignmentRunning(bool running) {
    if (m_alignmentRunning != running) {
        m_alignmentRunning = running;
        update();
    }
}

void PhonemeTextGridTierLabel::rebuildComboBox() {
    if (!m_tierCombo)
        return;

    // Block signals to avoid triggering currentIndexChanged during rebuild
    m_tierCombo->blockSignals(true);
    m_tierCombo->clear();

    if (!m_boundaryModel) {
        m_tierCombo->blockSignals(false);
        setFixedHeight(kTierRowHeight);
        return;
    }

    int tiers = m_boundaryModel->tierCount();
    setFixedHeight(kTierRowHeight);

    for (int t = 0; t < tiers; ++t) {
        QString name = m_boundaryModel->tierName(t);
        if (name.isEmpty())
            name = QStringLiteral("Tier %1").arg(t + 1);
        m_tierCombo->addItem(name);
    }

    if (m_activeTierIndex >= 0 && m_activeTierIndex < tiers)
        m_tierCombo->setCurrentIndex(m_activeTierIndex);
    else if (tiers > 0)
        m_tierCombo->setCurrentIndex(0);

    m_tierCombo->blockSignals(false);
    m_tierCombo->setVisible(tiers > 0);

    // Trigger resize to reposition combo
    if (auto *e = new QResizeEvent(size(), size())) {
        resizeEvent(e);
        delete e;
    }

    update();
}

} // namespace dstools
