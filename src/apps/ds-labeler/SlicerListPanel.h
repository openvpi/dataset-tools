/// @file SlicerListPanel.h
/// @brief Slice segment list panel for DsSlicerPage showing ID, duration, status.

#pragma once

#include <QListWidget>
#include <QWidget>
#include <vector>

namespace dstools {

/// @brief Displays slice segments in the slicer page with ID, duration, and status.
///
/// Shows each segment between adjacent slice points. Highlights segments
/// that are too long or too short. Supports right-click discard/restore.
class SlicerListPanel : public QWidget {
    Q_OBJECT

public:
    explicit SlicerListPanel(QWidget *parent = nullptr);
    ~SlicerListPanel() override = default;

    /// Rebuild the list from slice points and total duration.
    void setSliceData(const std::vector<double> &slicePoints, double totalDuration,
                      const QString &prefix = QStringLiteral("slice"));

    /// Mark a slice as discarded.
    void setDiscarded(int index, bool discarded);

    /// @return list of discarded slice indices.
    [[nodiscard]] std::vector<int> discardedIndices() const { return m_discarded; }

signals:
    void sliceDoubleClicked(int index, double startSec, double endSec);
    void discardToggled(int index, bool discarded);
    /// Request to add a slice point at the given time (seconds).
    void addSlicePointRequested(double timeSec);
    /// Request to remove the slice point at the given index in the slice points array.
    void removeSlicePointRequested(int pointIndex);

private:
    QListWidget *m_listWidget = nullptr;
    std::vector<double> m_slicePoints;
    double m_totalDuration = 0.0;
    std::vector<int> m_discarded;

    void rebuildList(const QString &prefix);
    void onContextMenu(const QPoint &pos);

    static constexpr double kMinDuration = 0.5;   // Warn if < 0.5s
    static constexpr double kMaxDuration = 30.0;   // Warn if > 30s
};

} // namespace dstools
