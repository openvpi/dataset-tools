#include "PhonemeTextGridTierLabel.h"

#include "IBoundaryModel.h"

#include <dstools/TimePos.h>

#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>

namespace dstools {

PhonemeTextGridTierLabel::PhonemeTextGridTierLabel(QWidget *parent)
    : TierLabelArea(parent) {
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_radioPanel = new QWidget(this);
    m_radioPanel->setFixedWidth(kRadioPanelWidth);
    m_radioPanel->setStyleSheet(QStringLiteral("background-color: #2a2a2f;"));

    m_radioLayout = new QVBoxLayout(m_radioPanel);
    m_radioLayout->setContentsMargins(2, 0, 2, 0);
    m_radioLayout->setSpacing(0);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);
    connect(m_buttonGroup, &QButtonGroup::idClicked, this, [this](int id) {
        setActiveTierIndex(id);
    });

    mainLayout->addWidget(m_radioPanel);
    mainLayout->addStretch(1);

    setFixedHeight(kTierRowHeight);
}

void PhonemeTextGridTierLabel::setBoundaryModel(IBoundaryModel *model) {
    TierLabelArea::setBoundaryModel(model);
    rebuildRadioButtons();
}

int PhonemeTextGridTierLabel::activeTierIndex() const {
    return m_activeTierIndex;
}

void PhonemeTextGridTierLabel::setActiveTierIndex(int index) {
    if (m_activeTierIndex != index) {
        m_activeTierIndex = index;
        if (m_buttonGroup && index >= 0 && index < m_radioButtons.size())
            m_radioButtons[index]->setChecked(true);
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

    painter.fillRect(rect(), QColor(40, 40, 45));

    if (!m_boundaryModel || m_boundaryModel->tierCount() == 0) {
        painter.setPen(QColor(128, 128, 128));
        painter.drawText(rect(), Qt::AlignCenter, tr("No label data"));
        return;
    }

    int tiers = m_boundaryModel->tierCount();
    int rowH = kTierRowHeight;
    int totalH = tiers * rowH;

    auto allBounds = allTierBoundaries();

    for (int t = 0; t < tiers; ++t) {
        int rowTop = t * rowH;
        int rowBottom = rowTop + rowH;

        bool isActive = (t == m_activeTierIndex);

        if (t > 0) {
            painter.setPen(QPen(QColor(60, 60, 70), 1));
            painter.drawLine(kRadioPanelWidth, rowTop, w, rowTop);
        }

        const auto &bounds = allBounds[t];
        for (double bTime : bounds) {
            int x = timeToX(bTime);
            if (x < kRadioPanelWidth || x > w)
                continue;

            if (isActive) {
                painter.setPen(QPen(QColor(255, 200, 100), 1));
            } else {
                painter.setPen(QPen(QColor(100, 100, 120), 1, Qt::DashLine));
            }
            painter.drawLine(x, rowTop, x, rowBottom);
        }

        if (!bounds.isEmpty() && isActive) {
            QFont font = painter.font();
            font.setPixelSize(10);
            painter.setFont(font);
            painter.setPen(QColor(180, 180, 200));

            std::vector<double> segBounds;
            segBounds.push_back(0.0);
            for (double b : bounds)
                segBounds.push_back(b);

            for (size_t i = 0; i + 1 < segBounds.size(); ++i) {
                int x1 = timeToX(segBounds[i]);
                int x2 = timeToX(segBounds[i + 1]);
                int segWidth = x2 - x1;
                if (segWidth > 30) {
                    QRect textRect(x1 + 2, rowTop, segWidth - 4, rowH);
                    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft,
                                     QString::number(i));
                }
            }
        }
    }

    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, 0, w, 0);
    painter.drawLine(0, totalH - 1, w, totalH - 1);
}

void PhonemeTextGridTierLabel::rebuildRadioButtons() {
    qDeleteAll(m_radioButtons);
    m_radioButtons.clear();

    if (!m_boundaryModel)
        return;

    int tiers = m_boundaryModel->tierCount();
    setFixedHeight(qMax(tiers, 1) * kTierRowHeight);

    for (int t = 0; t < tiers; ++t) {
        auto *btn = new QRadioButton(m_radioPanel);
        btn->setFixedSize(20, kTierRowHeight);
        if (t == m_activeTierIndex)
            btn->setChecked(true);
        m_buttonGroup->addButton(btn, t);
        m_radioLayout->addWidget(btn);
        m_radioButtons.append(btn);
    }

    m_radioPanel->setFixedHeight(tiers * kTierRowHeight);
    update();
}

} // namespace dstools
