/// @file BoundaryBindingManager.h
/// @brief Cross-tier boundary binding manager for synchronized boundary editing.

#pragma once

#include <QObject>
#include <QUndoCommand>
#include <vector>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

/// @brief Boundary position in a specific tier.
struct AlignedBoundary {
    int tierIndex;       ///< Tier containing the boundary.
    int boundaryIndex;   ///< Index of the boundary within the tier.
    double time;         ///< Time position of the boundary in seconds.
};

/// @brief Finds and links boundaries across tiers that are within a configurable
///        time tolerance, enabling synchronized multi-tier boundary moves.
class BoundaryBindingManager : public QObject {
    Q_OBJECT

public:
    /// @brief Constructs a binding manager for the given document.
    /// @param doc TextGrid document to manage bindings for.
    /// @param parent Optional parent QObject.
    explicit BoundaryBindingManager(TextGridDocument *doc, QObject *parent = nullptr);

    /// @brief Sets the alignment tolerance in milliseconds.
    /// @param toleranceMs Boundaries within this distance are considered aligned.
    void setTolerance(double toleranceMs = 1.0);

    /// @brief Returns the current alignment tolerance in milliseconds.
    [[nodiscard]] double tolerance() const { return m_tolerance; }

    /// @brief Enables or disables cross-tier binding.
    /// @param enabled True to enable binding.
    void setEnabled(bool enabled);

    /// @brief Returns whether binding is enabled.
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    /// @brief Finds all boundaries in other tiers aligned with the source boundary.
    /// @param sourceTierIndex Tier index of the source boundary.
    /// @param sourceBoundaryIndex Boundary index within the source tier.
    /// @return Vector of aligned boundaries in other tiers.
    [[nodiscard]] std::vector<AlignedBoundary> findAlignedBoundaries(
        int sourceTierIndex, int sourceBoundaryIndex) const;

    /// @brief Creates a linked move command for an aligned boundary group.
    /// @param sourceTierIndex Tier index of the source boundary.
    /// @param sourceBoundaryIndex Boundary index within the source tier.
    /// @param newTime New time position in seconds.
    /// @param doc TextGrid document to apply the move to.
    /// @return Undo command that moves all aligned boundaries together.
    [[nodiscard]] QUndoCommand *createLinkedMoveCommand(
        int sourceTierIndex, int sourceBoundaryIndex,
        double newTime, TextGridDocument *doc);

signals:
    /// @brief Emitted when binding state changes.
    void bindingChanged();

private:
    TextGridDocument *m_document = nullptr; ///< Associated document.
    double m_tolerance = 1.0;              ///< Alignment tolerance in ms.
    bool m_enabled = true;                 ///< Whether binding is active.
};

} // namespace phonemelabeler
} // namespace dstools
