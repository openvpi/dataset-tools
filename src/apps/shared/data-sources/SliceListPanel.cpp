#include "SliceListPanel.h"

#include <dstools/IEditorDataSource.h>
#include <dsfw/AppSettings.h>
#include <dsfw/widgets/FileProgressTracker.h>

#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMenu>

#include <algorithm>

namespace dstools {

SliceListPanel::SliceListPanel(QWidget *parent) : QWidget(parent) {
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setAlternatingRowColors(true);

    m_progressTracker = new dsfw::widgets::FileProgressTracker(
        dsfw::widgets::FileProgressTracker::LabelOnly, this);
    m_progressTracker->setFormat(QStringLiteral("%1 / %2 已标注 (%3%)"));
    m_progressTracker->setEmptyText(QStringLiteral("无切片"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_listWidget, 1);
    layout->addWidget(m_progressTracker);

    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, &SliceListPanel::onCurrentRowChanged);
}

// ── Editor mode ───────────────────────────────────────────────────────────────

void SliceListPanel::setDataSource(IEditorDataSource *source) {
    m_source = source;
    if (m_source) {
        connect(m_source, &IEditorDataSource::sliceListChanged,
                this, &SliceListPanel::refresh, Qt::UniqueConnection);
        connect(m_source, &IEditorDataSource::audioAvailabilityChanged,
                this, &SliceListPanel::refresh, Qt::UniqueConnection);
    }
    refresh();
}

void SliceListPanel::refresh() {
    QString previousId = currentSliceId();

    m_listWidget->blockSignals(true);
    m_listWidget->clear();

    if (m_source) {
        const auto ids = m_source->sliceIds();
        for (const auto &id : ids) {
            double dur = m_source->sliceDuration(id);
            QString text;
            if (dur > 0.0)
                text = QStringLiteral("%1  %2s").arg(id).arg(dur, 0, 'f', 1);
            else
                text = id;
            auto *item = new QListWidgetItem(text, m_listWidget);
            item->setData(Qt::UserRole, id);

            if (!m_source->audioExists(id)) {
                item->setForeground(QColor(200, 120, 50));  // orange
                item->setIcon(QIcon(QStringLiteral(":/icons/warning.svg")));
                item->setToolTip(
                    QStringLiteral("音频文件不存在: %1")
                        .arg(m_source->audioPath(id)));
            }
        }
    }

    m_listWidget->blockSignals(false);

    if (!previousId.isEmpty()) {
        for (int i = 0; i < m_listWidget->count(); ++i) {
            if (m_listWidget->item(i)->data(Qt::UserRole).toString() == previousId) {
                m_listWidget->setCurrentRow(i);
                return;
            }
        }
    }

    if (m_listWidget->count() > 0)
        m_listWidget->setCurrentRow(0);
}

void SliceListPanel::setCurrentSlice(const QString &sliceId) {
    for (int i = 0; i < m_listWidget->count(); ++i) {
        if (m_listWidget->item(i)->data(Qt::UserRole).toString() == sliceId) {
            m_listWidget->setCurrentRow(i);
            return;
        }
    }
}

QString SliceListPanel::currentSliceId() const {
    auto *item = m_listWidget->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void SliceListPanel::selectNext() {
    int row = m_listWidget->currentRow();
    if (row + 1 < m_listWidget->count())
        m_listWidget->setCurrentRow(row + 1);
}

void SliceListPanel::selectPrev() {
    int row = m_listWidget->currentRow();
    if (row > 0)
        m_listWidget->setCurrentRow(row - 1);
}

int SliceListPanel::sliceCount() const {
    return m_listWidget->count();
}

void SliceListPanel::onCurrentRowChanged(int row) {
    if (row < 0 || row >= m_listWidget->count())
        return;
    emit sliceSelected(m_listWidget->item(row)->data(Qt::UserRole).toString());
}

void SliceListPanel::setProgress(int completed, int total) {
    m_progressTracker->setProgress(completed, total);
}

dsfw::widgets::FileProgressTracker *SliceListPanel::progressTracker() const {
    return m_progressTracker;
}

void SliceListPanel::setSliceDirty(const QString &sliceId, bool dirty) {
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        if (item->data(Qt::UserRole).toString() != sliceId)
            continue;

        QFont font = item->font();
        font.setBold(dirty);
        item->setFont(font);

        QString baseText = item->data(Qt::UserRole + 1).toString();
        if (baseText.isEmpty()) {
            baseText = item->text();
            if (baseText.startsWith(QStringLiteral("* ")))
                baseText = baseText.mid(2);
            item->setData(Qt::UserRole + 1, baseText);
        }

        if (dirty) {
            item->setText(QStringLiteral("* ") + baseText);
        } else {
            item->setText(baseText);
        }
        break;
    }
}

QString SliceListPanel::ensureSelection(AppSettings &settings) {
    static const dstools::SettingsKey<QString> kLastSlice("State/lastSlice", "");

    // Try restoring last selected slice from settings
    QString lastSlice = settings.get(kLastSlice);
    if (!lastSlice.isEmpty()) {
        setCurrentSlice(lastSlice);
        // Check if it actually selected (might not exist in current list)
        if (currentSliceId() == lastSlice)
            return lastSlice;
    }

    // Fallback: select first item if available
    if (sliceCount() > 0) {
        QString firstId = currentSliceId();
        if (!firstId.isEmpty()) {
            // Force emit sliceSelected even if row is already set
            // (onCurrentRowChanged won't fire if row didn't change)
            emit sliceSelected(firstId);
            return firstId;
        }
    }

    return {};
}

void SliceListPanel::saveSelection(AppSettings &settings) const {
    static const dstools::SettingsKey<QString> kLastSlice("State/lastSlice", "");
    settings.set(kLastSlice, currentSliceId());
}

// ── Slicer mode ───────────────────────────────────────────────────────────────

void SliceListPanel::setSlicerMode(bool enabled) {
    if (m_slicerMode == enabled)
        return;
    m_slicerMode = enabled;
    if (enabled) {
        m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_listWidget, &QListWidget::customContextMenuRequested,
                this, &SliceListPanel::onContextMenu, Qt::UniqueConnection);
        // Qt::UniqueConnection is incompatible with lambdas; each lambda is a
        // unique functor type and Qt cannot deduplicate. Since setSlicerMode is
        // called once per panel instance, a plain connection is safe.
        connect(m_listWidget, &QListWidget::itemDoubleClicked, this,
                [this](QListWidgetItem *item) {
                    int row = m_listWidget->row(item);
                    double start = (row == 0) ? 0.0 : m_slicePoints[row - 1];
                    double end = (row < static_cast<int>(m_slicePoints.size()))
                                     ? m_slicePoints[row]
                                     : m_totalDuration;
                    emit sliceDoubleClicked(row, start, end);
                });
        m_progressTracker->hide();
    } else {
        m_listWidget->setContextMenuPolicy(Qt::NoContextMenu);
        m_progressTracker->show();
    }
}

void SliceListPanel::setSliceData(const std::vector<double> &slicePoints,
                                   double totalDuration, const QString &prefix) {
    m_slicePoints = slicePoints;
    m_totalDuration = totalDuration;
    m_discarded.clear();
    rebuildSlicerList(prefix);
}

void SliceListPanel::setDiscarded(int index, bool discarded) {
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

void SliceListPanel::rebuildSlicerList(const QString &prefix) {
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

void SliceListPanel::onContextMenu(const QPoint &pos) {
    if (!m_slicerMode)
        return;

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
