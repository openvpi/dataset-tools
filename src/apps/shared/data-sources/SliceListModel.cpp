#include "SliceListModel.h"

#include <dstools/IEditorDataSource.h>
#include <dsfw/AppSettings.h>
#include "SharedSettingsKeys.h"

#include <QColor>
#include <QIcon>

#include <algorithm>

namespace dstools {

SliceListModel::SliceListModel(QObject *parent) : QAbstractListModel(parent) {}

int SliceListModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_items.size());
}

QVariant SliceListModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size()))
        return {};

    const auto &item = m_items[index.row()];

    switch (role) {
    case Qt::DisplayRole:
        return buildDisplayText(index.row());
    case Qt::ForegroundRole: {
        if (item.discarded)
            return QColor(Qt::gray);
        if (item.dirty)
            return QColor(255, 152, 0);
        if (!item.audioExists)
            return QColor(200, 120, 50);
        return {};
    }
    case Qt::ToolTipRole: {
        if (!item.audioExists)
            return QStringLiteral("音频文件不存在: %1").arg(item.id);
        return {};
    }
    case Qt::DecorationRole: {
        if (!item.audioExists)
            return QIcon(QStringLiteral(":/icons/warning.svg"));
        return {};
    }
    case SliceIdRole:
        return item.id;
    case DurationRole:
        return item.duration;
    case BaseTextRole:
        return item.baseText;
    case DirtyRole:
        return item.dirty;
    case AudioExistsRole:
        return item.audioExists;
    case DirtyLayersRole:
        return item.dirtyLayers;
    case DiscardedRole:
        return item.discarded;
    case StartTimeRole:
        return item.startTime;
    case EndTimeRole:
        return item.endTime;
    default:
        return {};
    }
}

void SliceListModel::setDataSource(IEditorDataSource *source) {
    m_source = source;
    if (m_source) {
        connect(m_source, &IEditorDataSource::sliceListChanged,
                this, &SliceListModel::refresh, Qt::UniqueConnection);
        connect(m_source, &IEditorDataSource::audioAvailabilityChanged,
                this, &SliceListModel::refresh, Qt::UniqueConnection);
    }
    refresh();
}

void SliceListModel::refresh() {
    beginResetModel();
    rebuildEditorItems();
    endResetModel();
}

void SliceListModel::setCurrentSlice(const QString &sliceId) {
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].id == sliceId) {
            m_currentIndex = i;
            return;
        }
    }
    m_currentIndex = -1;
}

QString SliceListModel::currentSliceId() const {
    if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_items.size()))
        return m_items[m_currentIndex].id;
    return {};
}

void SliceListModel::selectNext() {
    if (m_currentIndex + 1 < static_cast<int>(m_items.size()))
        m_currentIndex++;
}

void SliceListModel::selectPrev() {
    if (m_currentIndex > 0)
        m_currentIndex--;
}

int SliceListModel::sliceCount() const {
    return static_cast<int>(m_items.size());
}

void SliceListModel::setSliceDirty(const QString &sliceId, bool dirty) {
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].id != sliceId)
            continue;

        m_items[i].dirty = dirty;
        rebuildItemText(i);
        emit dataChanged(index(i), index(i), {Qt::DisplayRole, Qt::ForegroundRole, Qt::ToolTipRole});
        break;
    }
}

void SliceListModel::setSliceDirtyLayers(const QString &sliceId, const QStringList &dirtyLayers) {
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].id != sliceId)
            continue;

        m_items[i].dirty = !dirtyLayers.isEmpty();
        m_items[i].dirtyLayers = dirtyLayers;
        rebuildItemText(i);
        emit dataChanged(index(i), index(i), {Qt::DisplayRole, Qt::ForegroundRole, Qt::ToolTipRole});
        break;
    }
}

QString SliceListModel::ensureSelection(AppSettings &settings) {
    static const SettingsKey<QString> kLastSlice("State/lastSlice", "");

    QString lastSlice = settings.get(kLastSlice);
    if (!lastSlice.isEmpty()) {
        setCurrentSlice(lastSlice);
        if (currentSliceId() == lastSlice)
            return lastSlice;
    }

    if (sliceCount() > 0) {
        m_currentIndex = 0;
        QString firstId = currentSliceId();
        if (!firstId.isEmpty()) {
            emit currentSliceChanged(firstId);
            return firstId;
        }
    }

    return {};
}

void SliceListModel::saveSelection(AppSettings &settings) const {
    static const SettingsKey<QString> kLastSlice("State/lastSlice", "");
    settings.set(kLastSlice, currentSliceId());
}

void SliceListModel::setSlicerMode(bool enabled) {
    if (m_slicerMode == enabled)
        return;
    m_slicerMode = enabled;
}

void SliceListModel::setSliceData(const std::vector<double> &slicePoints,
                                   double totalDuration, const QString &prefix) {
    beginResetModel();
    m_items.clear();
    m_discardedInSlicer.clear();
    m_currentIndex = -1;

    int numSegments = static_cast<int>(slicePoints.size()) + 1;
    for (int i = 0; i < numSegments; ++i) {
        double start = (i == 0) ? 0.0 : slicePoints[i - 1];
        double end = (i < static_cast<int>(slicePoints.size())) ? slicePoints[i]
                                                                 : totalDuration;
        double duration = end - start;

        Item item;
        item.id = QStringLiteral("%1_%2").arg(prefix).arg(i + 1, 3, 10, QChar('0'));
        item.startTime = start;
        item.endTime = end;
        item.duration = duration;
        m_items.push_back(item);
    }
    endResetModel();
}

void SliceListModel::setDiscarded(int index, bool discarded) {
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;

    auto it = std::find(m_discardedInSlicer.begin(), m_discardedInSlicer.end(), index);
    if (discarded && it == m_discardedInSlicer.end()) {
        m_discardedInSlicer.push_back(index);
    } else if (!discarded && it != m_discardedInSlicer.end()) {
        m_discardedInSlicer.erase(it);
    }

    m_items[index].discarded = discarded;
    emit dataChanged(this->index(index), this->index(index), {Qt::DisplayRole, Qt::ForegroundRole});
}

std::vector<int> SliceListModel::discardedIndices() const {
    return m_discardedInSlicer;
}

double SliceListModel::sliceStart(int index) const {
    if (index >= 0 && index < static_cast<int>(m_items.size()))
        return m_items[index].startTime;
    return 0.0;
}

double SliceListModel::sliceEnd(int index) const {
    if (index >= 0 && index < static_cast<int>(m_items.size()))
        return m_items[index].endTime;
    return 0.0;
}

void SliceListModel::rebuildEditorItems() {
    m_items.clear();
    m_currentIndex = -1;

    if (!m_source)
        return;

    const auto ids = m_source->sliceIds();
    for (const auto &id : ids) {
        double dur = m_source->sliceDuration(id);
        Item item;
        item.id = id;
        item.duration = dur;
        item.audioExists = m_source->audioExists(id);
        m_items.push_back(item);
    }
}

void SliceListModel::rebuildItemText(int index) {
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;

    auto &item = m_items[index];
    if (item.baseText.isEmpty()) {
        item.baseText = buildEditorText(item);
    }
}

QString SliceListModel::buildDisplayText(int index) const {
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return {};

    const auto &item = m_items[index];

    if (m_slicerMode) {
        double duration = item.endTime - item.startTime;
        QString status;
        if (item.discarded)
            status = QStringLiteral("🗑 已丢弃");
        else if (duration < kMinDuration)
            status = QStringLiteral("⚠ 过短");
        else if (duration > kMaxDuration)
            status = QStringLiteral("⚠ 过长");
        else
            status = QStringLiteral("✓");

        return QStringLiteral("%1    %2    %3s~%4s    %5s    %6")
            .arg(index + 1, 3)
            .arg(item.id)
            .arg(item.startTime, 0, 'f', 2)
            .arg(item.endTime, 0, 'f', 2)
            .arg(duration, 0, 'f', 2)
            .arg(status);
    }

    QString base = item.baseText.isEmpty() ? buildEditorText(item) : item.baseText;
    if (item.dirty)
        base += QStringLiteral(" ●");
    return base;
}

QString SliceListModel::buildEditorText(const Item &item) const {
    if (item.duration > 0.0)
        return QStringLiteral("%1  %2s").arg(item.id).arg(item.duration, 0, 'f', 1);
    return item.id;
}

} // namespace dstools