#include "SliceCommands.h"

#include <algorithm>

namespace dstools {

AddSlicePointCommand::AddSlicePointCommand(std::vector<double> &points, double timeSec,
                                           std::function<void()> refresh)
    : QUndoCommand(QObject::tr("Add slice point")),
      m_points(points),
      m_time(timeSec),
      m_refresh(std::move(refresh)) {
    m_insertIndex =
        static_cast<int>(std::lower_bound(m_points.begin(), m_points.end(), m_time) -
                         m_points.begin());
}

void AddSlicePointCommand::undo() {
    m_points.erase(m_points.begin() + m_insertIndex);
    if (m_refresh)
        m_refresh();
}

void AddSlicePointCommand::redo() {
    m_points.insert(m_points.begin() + m_insertIndex, m_time);
    if (m_refresh)
        m_refresh();
}

MoveSlicePointCommand::MoveSlicePointCommand(std::vector<double> &points, int index,
                                             double oldTime, double newTime,
                                             std::function<void()> refresh)
    : QUndoCommand(QObject::tr("Move slice point")),
      m_points(points),
      m_index(index),
      m_oldTime(oldTime),
      m_newTime(newTime),
      m_refresh(std::move(refresh)) {}

void MoveSlicePointCommand::undo() {
    m_points[m_index] = m_oldTime;
    std::sort(m_points.begin(), m_points.end());
    if (m_refresh)
        m_refresh();
}

void MoveSlicePointCommand::redo() {
    m_points[m_index] = m_newTime;
    std::sort(m_points.begin(), m_points.end());
    if (m_refresh)
        m_refresh();
}

RemoveSlicePointCommand::RemoveSlicePointCommand(std::vector<double> &points, int index,
                                                 std::function<void()> refresh)
    : QUndoCommand(QObject::tr("Remove slice point")),
      m_points(points),
      m_index(index),
      m_time(points[index]),
      m_refresh(std::move(refresh)) {}

void RemoveSlicePointCommand::undo() {
    m_points.insert(m_points.begin() + m_index, m_time);
    if (m_refresh)
        m_refresh();
}

void RemoveSlicePointCommand::redo() {
    m_points.erase(m_points.begin() + m_index);
    if (m_refresh)
        m_refresh();
}

} // namespace dstools
