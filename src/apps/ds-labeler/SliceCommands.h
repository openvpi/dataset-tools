/// @file SliceCommands.h
/// @brief QUndoCommand implementations for slice point operations.

#pragma once

#include <QUndoCommand>
#include <vector>

namespace dstools {

class DsSlicerPage;

/// @brief Add a slice point at a given time.
class AddSlicePointCommand : public QUndoCommand {
public:
    AddSlicePointCommand(std::vector<double> &points, double timeSec,
                         std::function<void()> refresh);
    void undo() override;
    void redo() override;

private:
    std::vector<double> &m_points;
    double m_time;
    int m_insertIndex = 0;
    std::function<void()> m_refresh;
};

/// @brief Move a slice point from one time to another.
class MoveSlicePointCommand : public QUndoCommand {
public:
    MoveSlicePointCommand(std::vector<double> &points, int index, double oldTime,
                          double newTime, std::function<void()> refresh);
    void undo() override;
    void redo() override;

private:
    std::vector<double> &m_points;
    int m_index;
    double m_oldTime;
    double m_newTime;
    std::function<void()> m_refresh;
};

/// @brief Remove a slice point.
class RemoveSlicePointCommand : public QUndoCommand {
public:
    RemoveSlicePointCommand(std::vector<double> &points, int index,
                            std::function<void()> refresh);
    void undo() override;
    void redo() override;

private:
    std::vector<double> &m_points;
    int m_index;
    double m_time;
    std::function<void()> m_refresh;
};

} // namespace dstools
