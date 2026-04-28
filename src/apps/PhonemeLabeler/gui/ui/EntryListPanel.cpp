#include "EntryListPanel.h"
#include "TextGridDocument.h"
#include <dstools/ViewportController.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QWheelEvent>
#include <QListWidgetItem>

namespace dstools {
namespace phonemelabeler {

EntryListPanel::EntryListPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    auto *label = new QLabel(tr("Entries"), this);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("QLabel { font-weight: bold; padding: 4px; }");
    layout->addWidget(label);

    m_listWidget = new QListWidget(this);
    m_listWidget->setFocusPolicy(Qt::NoFocus); // parent handles wheel
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, &EntryListPanel::onCurrentRowChanged);
}

void EntryListPanel::setDocument(TextGridDocument *doc) {
    m_document = doc;
    rebuildEntries();
}

void EntryListPanel::setViewportController(ViewportController *viewport) {
    m_viewport = viewport;
}

void EntryListPanel::rebuildEntries() {
    m_updatingSelection = true;

    // Preserve current selection
    int previousIndex = currentEntryIndex();

    m_listWidget->clear();

    if (!m_document) {
        m_updatingSelection = false;
        return;
    }

    int tier = m_document->activeTierIndex();
    if (tier < 0 || tier >= m_document->tierCount() || !m_document->isIntervalTier(tier)) {
        m_updatingSelection = false;
        return;
    }

    int count = m_document->intervalCount(tier);
    for (int i = 0; i < count; ++i) {
        QString text = m_document->intervalText(tier, i);
        double start = m_document->intervalStart(tier, i);
        double end = m_document->intervalEnd(tier, i);

        QString display;
        if (text.isEmpty()) {
            display = QString("[%1] (%2s - %3s)")
                .arg(i)
                .arg(start, 0, 'f', 3)
                .arg(end, 0, 'f', 3);
        } else {
            display = QString("[%1] %2  (%3s - %4s)")
                .arg(i)
                .arg(text)
                .arg(start, 0, 'f', 3)
                .arg(end, 0, 'f', 3);
        }

        auto *item = new QListWidgetItem(display, m_listWidget);
        item->setData(Qt::UserRole, i);         // interval index
        item->setData(Qt::UserRole + 1, start); // start time
        item->setData(Qt::UserRole + 2, end);   // end time
    }

    // Restore previous selection, or default to first entry
    if (count > 0) {
        int restoreIndex = (previousIndex >= 0 && previousIndex < count) ? previousIndex : 0;
        m_listWidget->setCurrentRow(restoreIndex);
    }

    m_updatingSelection = false;
}

void EntryListPanel::selectEntry(int index) {
    if (index >= 0 && index < m_listWidget->count()) {
        m_listWidget->setCurrentRow(index);
    }
}

int EntryListPanel::currentEntryIndex() const {
    int row = m_listWidget->currentRow();
    if (row < 0) return -1;
    auto *item = m_listWidget->item(row);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void EntryListPanel::wheelEvent(QWheelEvent *event) {
    int delta = event->angleDelta().y();
    if (delta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    int currentRow = m_listWidget->currentRow();
    int count = m_listWidget->count();
    if (count == 0) return;

    int newRow;
    if (delta < 0) {
        // Scroll down → next entry
        newRow = (currentRow + 1 < count) ? currentRow + 1 : currentRow;
    } else {
        // Scroll up → previous entry
        newRow = (currentRow > 0) ? currentRow - 1 : 0;
    }

    if (newRow != currentRow) {
        m_listWidget->setCurrentRow(newRow);
    }

    event->accept();
}

void EntryListPanel::onCurrentRowChanged(int row) {
    if (m_updatingSelection || row < 0 || !m_document) return;

    auto *item = m_listWidget->item(row);
    if (!item) return;

    int intervalIndex = item->data(Qt::UserRole).toInt();
    double start = item->data(Qt::UserRole + 1).toDouble();
    double end = item->data(Qt::UserRole + 2).toDouble();
    int tier = m_document->activeTierIndex();

    centerEntryInViewport(intervalIndex);

    // Scroll list widget to show selected item
    m_listWidget->scrollToItem(item, QAbstractItemView::PositionAtCenter);

    emit entrySelected(tier, intervalIndex, start, end);
}

void EntryListPanel::centerEntryInViewport(int intervalIndex) {
    if (!m_document || !m_viewport) return;

    int tier = m_document->activeTierIndex();
    if (tier < 0 || intervalIndex < 0 || intervalIndex >= m_document->intervalCount(tier))
        return;

    double start = m_document->intervalStart(tier, intervalIndex);
    double end = m_document->intervalEnd(tier, intervalIndex);
    double entryCenter = (start + end) / 2.0;

    // Center the viewport on the entry
    double viewDuration = m_viewport->duration();
    double newStart = entryCenter - viewDuration / 2.0;
    double newEnd = entryCenter + viewDuration / 2.0;

    m_viewport->setViewRange(newStart, newEnd);
}

} // namespace phonemelabeler
} // namespace dstools
