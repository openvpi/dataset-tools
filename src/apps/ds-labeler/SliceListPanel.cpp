#include "SliceListPanel.h"
#include "ProjectDataSource.h"

#include <dsfw/widgets/FileProgressTracker.h>

namespace dstools {

SliceListPanel::SliceListPanel(QWidget *parent) : QWidget(parent) {
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

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

void SliceListPanel::setDataSource(ProjectDataSource *source) {
    m_source = source;
    if (m_source) {
        connect(m_source, &ProjectDataSource::sliceListChanged,
                this, &SliceListPanel::refresh, Qt::UniqueConnection);
    }
    refresh();
}

void SliceListPanel::refresh() {
    m_listWidget->blockSignals(true);
    m_listWidget->clear();

    if (m_source) {
        const auto ids = m_source->sliceIds();
        for (const auto &id : ids) {
            auto *item = new QListWidgetItem(id, m_listWidget);
            item->setData(Qt::UserRole, id);
        }
    }

    m_listWidget->blockSignals(false);

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

} // namespace dstools
