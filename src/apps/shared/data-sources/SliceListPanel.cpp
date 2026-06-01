#include "SliceListPanel.h"

#include <dstools/IEditorDataSource.h>
#include <dsfw/AppSettings.h>
#include <dsfw/widgets/FileProgressTracker.h>

#include <QAction>
#include <QMenu>

namespace dstools {

SliceListPanel::SliceListPanel(QWidget *parent) : QWidget(parent) {
    m_model = new SliceListModel(this);

    m_listView = new QListView(this);
    m_listView->setModel(m_model);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setAlternatingRowColors(true);

    m_progressTracker = new dsfw::widgets::FileProgressTracker(
        dsfw::widgets::FileProgressTracker::LabelOnly, this);
    m_progressTracker->setFormat(QStringLiteral("%1 / %2 已标注 (%3%)"));
    m_progressTracker->setEmptyText(QStringLiteral("无切片"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_listView, 1);
    layout->addWidget(m_progressTracker);

    connect(m_listView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &SliceListPanel::onCurrentRowChanged);
    connect(m_model, &SliceListModel::currentSliceChanged,
            this, &SliceListPanel::sliceSelected);

    connect(m_model, &SliceListModel::dataChanged, this, [this]() {
        m_listView->viewport()->update();
    });
}

void SliceListPanel::setDataSource(IEditorDataSource *source) {
    m_source = source;
    m_model->setDataSource(source);
}

void SliceListPanel::refresh() {
    QString previousId = currentSliceId();

    m_model->refresh();

    if (!previousId.isEmpty()) {
        setCurrentSlice(previousId);
        if (currentSliceId() == previousId)
            return;
    }

    if (sliceCount() > 0) {
        QModelIndex firstIdx = m_model->index(0, 0);
        m_listView->selectionModel()->setCurrentIndex(firstIdx, QItemSelectionModel::SelectCurrent);
    }
}

void SliceListPanel::setCurrentSlice(const QString &sliceId) {
    m_model->setCurrentSlice(sliceId);
    for (int i = 0; i < m_model->sliceCount(); ++i) {
        QModelIndex idx = m_model->index(i, 0);
        if (idx.data(SliceListModel::SliceIdRole).toString() == sliceId) {
            m_listView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::SelectCurrent);
            return;
        }
    }
}

QString SliceListPanel::currentSliceId() const {
    QModelIndex current = m_listView->selectionModel()->currentIndex();
    return current.data(SliceListModel::SliceIdRole).toString();
}

void SliceListPanel::selectNext() {
    int row = m_listView->selectionModel()->currentIndex().row();
    if (row + 1 < m_model->sliceCount()) {
        QModelIndex next = m_model->index(row + 1, 0);
        m_listView->selectionModel()->setCurrentIndex(next, QItemSelectionModel::SelectCurrent);
    }
}

void SliceListPanel::selectPrev() {
    int row = m_listView->selectionModel()->currentIndex().row();
    if (row > 0) {
        QModelIndex prev = m_model->index(row - 1, 0);
        m_listView->selectionModel()->setCurrentIndex(prev, QItemSelectionModel::SelectCurrent);
    }
}

int SliceListPanel::sliceCount() const {
    return m_model->sliceCount();
}

void SliceListPanel::onCurrentRowChanged(const QModelIndex &current, const QModelIndex & /*previous*/) {
    if (!current.isValid())
        return;
    emit sliceSelected(current.data(SliceListModel::SliceIdRole).toString());
}

void SliceListPanel::setProgress(int completed, int total) {
    m_progressTracker->setProgress(completed, total);
}

dsfw::widgets::FileProgressTracker *SliceListPanel::progressTracker() const {
    return m_progressTracker;
}

void SliceListPanel::setSliceDirty(const QString &sliceId, bool dirty) {
    m_model->setSliceDirty(sliceId, dirty);
}

void SliceListPanel::setSliceDirtyLayers(const QString &sliceId, const QStringList &dirtyLayers) {
    m_model->setSliceDirtyLayers(sliceId, dirtyLayers);
}

void SliceListPanel::setSliceLoadError(const QString &sliceId, const QString &error) {
    m_model->setSliceLoadError(sliceId, error);
}

QString SliceListPanel::ensureSelection(AppSettings &settings) {
    return m_model->ensureSelection(settings);
}

void SliceListPanel::saveSelection(AppSettings &settings) const {
    m_model->saveSelection(settings);
}

void SliceListPanel::setSlicerMode(bool enabled) {
    if (m_slicerMode == enabled)
        return;
    m_slicerMode = enabled;
    m_model->setSlicerMode(enabled);

    if (enabled) {
        m_listView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_listView, &QListView::customContextMenuRequested,
                this, &SliceListPanel::onContextMenu, Qt::UniqueConnection);

        connect(m_listView, &QListView::doubleClicked, this,
                [this](const QModelIndex &index) {
                    int row = index.row();
                    double start = (row == 0) ? 0.0 : m_slicePoints[row - 1];
                    double end = (row < static_cast<int>(m_slicePoints.size()))
                                     ? m_slicePoints[row]
                                     : m_totalDuration;
                    emit sliceDoubleClicked(row, start, end);
                });
        m_progressTracker->hide();
    } else {
        m_listView->setContextMenuPolicy(Qt::NoContextMenu);
        m_progressTracker->show();
    }
}

void SliceListPanel::setSliceData(const std::vector<double> &slicePoints,
                                   double totalDuration, const QString &prefix) {
    m_slicePoints = slicePoints;
    m_totalDuration = totalDuration;
    m_model->setSliceData(slicePoints, totalDuration, prefix);
}

void SliceListPanel::setDiscarded(int index, bool discarded) {
    m_model->setDiscarded(index, discarded);

    if (discarded) {
        auto discardedList = m_model->discardedIndices();
        QModelIndex idx = m_model->index(index, 0);
        m_listView->selectionModel()->clearSelection();
        if (m_model->sliceCount() > 0) {
            m_listView->selectionModel()->setCurrentIndex(m_model->index(0, 0),
                                                           QItemSelectionModel::SelectCurrent);
        }
    }

    emit discardToggled(index, discarded);
}

void SliceListPanel::onContextMenu(const QPoint &pos) {
    if (!m_slicerMode)
        return;

    QModelIndex idx = m_listView->indexAt(pos);
    if (!idx.isValid())
        return;

    int row = idx.row();
    bool disc = false;
    auto discardedList = m_model->discardedIndices();
    disc = std::find(discardedList.begin(), discardedList.end(), row) != discardedList.end();

    double segStart = m_model->sliceStart(row);
    double segEnd = m_model->sliceEnd(row);

    QMenu menu(this);

    double midpoint = (segStart + segEnd) / 2.0;
    menu.addAction(tr("Add slice point here (midpoint)"), this, [this, midpoint]() {
        emit addSlicePointRequested(midpoint);
    });

    if (row > 0) {
        menu.addAction(tr("Remove left boundary"), this, [this, row]() {
            emit removeSlicePointRequested(row - 1);
        });
    }

    if (row < static_cast<int>(m_slicePoints.size())) {
        menu.addAction(tr("Remove right boundary"), this, [this, row]() {
            emit removeSlicePointRequested(row);
        });
    }

    menu.addSeparator();

    if (disc) {
        menu.addAction(tr("Restore slice"), this, [this, row]() { setDiscarded(row, false); });
    } else {
        menu.addAction(tr("Discard slice"), this, [this, row]() { setDiscarded(row, true); });
    }
    menu.exec(m_listView->mapToGlobal(pos));
}

} // namespace dstools