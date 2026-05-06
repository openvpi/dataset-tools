/// @file BoundaryDragController.h
/// @brief Centralized boundary drag state machine for all chart widgets.
///
/// Extracts the duplicated startDrag/updateDrag/endDrag logic from
/// WaveformWidget, SpectrogramWidget, PowerWidget, IntervalTierView
/// into a single controller owned by AudioVisualizerContainer.

#pragma once

#include <QObject>
#include <vector>

#include <dstools/TimePos.h>

class QUndoStack;

namespace dstools {
namespace phonemelabeler {

class IBoundaryModel;

/// @brief A boundary reference for cross-tier binding.
struct AlignedBoundary {
    int tierIndex;
    int boundaryIndex;
    TimePos time;
};

/// @brief Centralized boundary drag controller.
///
/// Owns the drag state machine (start → update → end/cancel) and binding
/// partner discovery. Widgets call into this controller instead of
/// implementing their own drag logic.
class BoundaryDragController : public QObject {
    Q_OBJECT

public:
    explicit BoundaryDragController(QObject *parent = nullptr);

    // ── Configuration ────────────────────────────────────────────────

    void setBindingEnabled(bool enabled);
    void setSnapEnabled(bool enabled);
    void setToleranceUs(TimePos toleranceUs);

    [[nodiscard]] bool isBindingEnabled() const { return m_bindingEnabled; }
    [[nodiscard]] bool isSnapEnabled() const { return m_snapEnabled; }

    // ── Drag state ───────────────────────────────────────────────────

    [[nodiscard]] bool isDragging() const { return m_dragging; }
    [[nodiscard]] int draggedTier() const { return m_draggedTier; }
    [[nodiscard]] int draggedBoundary() const { return m_draggedBoundary; }

    /// Begin a boundary drag. Discovers binding partners by time proximity.
    void startDrag(int tierIndex, int boundaryIndex, IBoundaryModel *model);

    /// Update drag preview. Clamps source + partners, applies preview moves.
    void updateDrag(TimePos proposedTime);

    /// End drag. Snaps if enabled, restores originals, pushes undo command.
    void endDrag(TimePos finalTime, QUndoStack *undoStack);

    /// Cancel drag without committing. Restores all boundaries to originals.
    void cancelDrag();

signals:
    void dragStarted(int tierIndex, int boundaryIndex);
    void dragging(int tierIndex, int boundaryIndex, TimePos currentTime);
    void dragFinished(int tierIndex, int boundaryIndex, TimePos finalTime);
    void bindingEnabledChanged(bool enabled);
    void snapEnabledChanged(bool enabled);

private:
    /// Find boundaries in other tiers within tolerance of the source boundary.
    std::vector<AlignedBoundary> findPartners(int sourceTier, int sourceBoundaryIndex) const;

    /// Restore all boundaries (source + partners) to their original positions.
    void restoreOriginals();

    bool m_bindingEnabled = true;
    bool m_snapEnabled = true;
    TimePos m_toleranceUs = 1000; // 1ms default

    bool m_dragging = false;
    int m_draggedTier = -1;
    int m_draggedBoundary = -1;
    TimePos m_dragStartTime = 0;
    IBoundaryModel *m_model = nullptr;

    std::vector<AlignedBoundary> m_partners;
    std::vector<TimePos> m_partnerStartTimes;
};

} // namespace phonemelabeler
} // namespace dstools
