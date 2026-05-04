#include "SlicerListPanel.h"

#include <QAction>
#include <QHBoxLayout>
#include <QMenu>
#include <QVBoxLayout>

namespace dstools {

SlicerListPanel::SlicerListPanel(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_listWidget = new QListWidget(this);
    m_listWidget->setAlternatingRowColors(true);
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::customContextMenuRequested, this,
            &SlicerListPanel::onContextMenu);
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) {
                int row = m_listWidget->row(item);
                double start = (row == 0) ? 0.0 : m_slicePoints[row - 1];
                double end = (row < static_cast<int>(m_slicePoints.size()))
                                 ? m_slicePoints[row]
                                 : m_totalDuration;
                emit sliceDoubleClicked(row, start, end);
            });
}

void SlicerListPanel::setSliceData(const std::vector<double> &slicePoints,
                                    double totalDuration, const QString &prefix) {
    m_slicePoints = slicePoints;
    m_totalDuration = totalDuration;
    m_discarded.clear();
    rebuildList(prefix);
}

void SlicerListPanel::setDiscarded(int index, bool discarded) {
    auto it = std::find(m_discarded.begin(), m_discarded.end(), index);
    if (discarded && it == m_discarded.end()) {
        m_discarded.push_back(index);
    } else if (!discarded && it != m_discarded.end()) {
        m_discarded.erase(it);
    }

    if (index >= 0 && index < m_listWidget->count()) {
        auto *item = m_listWidget->item(index);
        if (discarded) {
            item->setForeground(Qt::gray);
            item->setText(item->text().replace(QStringLiteral("✓"), QStringLiteral("🗑 已丢弃")));
        } else {
            item->setForeground(palette().text().color());
        }
    }

    emit discardToggled(index, discarded);
}

void SlicerListPanel::rebuildList(const QString &prefix) {
    m_listWidget->clear();

    int numSegments = static_cast<int>(m_slicePoints.size()) + 1;
    for (int i = 0; i < numSegments; ++i) {
        double start = (i == 0) ? 0.0 : m_slicePoints[i - 1];
        double end = (i < static_cast<int>(m_slicePoints.size())) ? m_slicePoints[i]
                                                                   : m_totalDuration;
        double duration = end - start;

        QString id = QStringLiteral("%1_%2")
                         .arg(prefix)
                         .arg(i + 1, 3, 10, QChar('0'));

        QString status = QStringLiteral("✓");
        if (duration < kMinDuration)
            status = QStringLiteral("⚠ 过短");
        else if (duration > kMaxDuration)
            status = QStringLiteral("⚠ 过长");

        bool disc = std::find(m_discarded.begin(), m_discarded.end(), i) != m_discarded.end();
        if (disc)
            status = QStringLiteral("🗑 已丢弃");

        QString text = QStringLiteral("%1    %2    %3s    %4")
                           .arg(i + 1, 3)
                           .arg(id)
                           .arg(duration, 0, 'f', 1)
                           .arg(status);

        auto *item = new QListWidgetItem(text);
        if (disc)
            item->setForeground(Qt::gray);
        else if (duration < kMinDuration || duration > kMaxDuration)
            item->setForeground(QColor(255, 180, 80));
        m_listWidget->addItem(item);
    }
}

void SlicerListPanel::onContextMenu(const QPoint &pos) {
    auto *item = m_listWidget->itemAt(pos);
    if (!item)
        return;

    int row = m_listWidget->row(item);
    bool disc = std::find(m_discarded.begin(), m_discarded.end(), row) != m_discarded.end();

    double segStart = (row == 0) ? 0.0 : m_slicePoints[row - 1];
    double segEnd = (row < static_cast<int>(m_slicePoints.size()))
                        ? m_slicePoints[row]
                        : m_totalDuration;

    QMenu menu(this);

    double midpoint = (segStart + segEnd) / 2.0;
    menu.addAction(tr("Add slice point here (midpoint)"), [this, midpoint]() {
        emit addSlicePointRequested(midpoint);
    });

    if (row > 0) {
        menu.addAction(tr("Remove left boundary"), [this, row]() {
            emit removeSlicePointRequested(row - 1);
        });
    }

    if (row < static_cast<int>(m_slicePoints.size())) {
        menu.addAction(tr("Remove right boundary"), [this, row]() {
            emit removeSlicePointRequested(row);
        });
    }

    menu.addSeparator();

    if (disc) {
        menu.addAction(tr("Restore slice"), [this, row]() { setDiscarded(row, false); });
    } else {
        menu.addAction(tr("Discard slice"), [this, row]() { setDiscarded(row, true); });
    }
    menu.exec(m_listWidget->mapToGlobal(pos));
}

} // namespace dstools
