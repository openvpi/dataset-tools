/// @file IBoundaryModel.h
/// @brief Abstract boundary model interface for visualization widgets.
///
/// Decouples WaveformWidget/SpectrogramWidget from TextGridDocument so that
/// any boundary source (TextGrid multi-tier, simple slice points, etc.) can
/// drive the same visualization and interaction logic.

#pragma once

#include <dstools/TimePos.h>
#include <QString>

namespace dstools {
namespace phonemelabeler {

/// @brief Abstract interface for providing boundary data to visualization widgets.
///
/// Implementations:
///   - TextGridDocument (multi-tier phoneme boundaries with binding)
///   - SliceBoundaryModel (single-tier slice points, no binding)
class IBoundaryModel {
public:
    virtual ~IBoundaryModel() = default;

    /// @brief Number of tiers in this model.
    [[nodiscard]] virtual int tierCount() const = 0;

    /// @brief Display name for a tier (e.g. "phoneme", "grapheme").
    /// Default returns empty string.
    [[nodiscard]] virtual QString tierName(int /*tierIndex*/) const { return {}; }

    /// @brief Index of the currently active tier (for hit-testing and rendering).
    [[nodiscard]] virtual int activeTierIndex() const = 0;

    /// @brief Number of boundaries in the given tier.
    [[nodiscard]] virtual int boundaryCount(int tierIndex) const = 0;

    /// @brief Time position of a boundary (in microseconds).
    [[nodiscard]] virtual TimePos boundaryTime(int tierIndex, int boundaryIndex) const = 0;

    /// @brief Move a boundary to a new time. May be called during drag preview.
    virtual void moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) = 0;

    /// @brief Total duration in microseconds.
    [[nodiscard]] virtual TimePos totalDuration() const = 0;

    /// @brief Whether this model supports boundary binding (cross-tier sync).
    [[nodiscard]] virtual bool supportsBinding() const { return false; }

    /// @brief Whether this model supports boundary insertion via click.
    [[nodiscard]] virtual bool supportsInsert() const { return false; }

    /// @brief Clamp a boundary time to valid range for the given tier.
    /// Ensures the boundary does not cross adjacent boundaries in the same tier.
    /// @param tierIndex The tier index.
    /// @param boundaryIndex The boundary index being dragged.
    /// @param proposedTime The proposed new time (in microseconds).
    /// @return The clamped time (in microseconds).
    [[nodiscard]] virtual TimePos clampBoundaryTime(int /*tierIndex*/, int /*boundaryIndex*/, TimePos proposedTime) const {
        return proposedTime;
    }

    /// @brief Snap a boundary time to the nearest lower-tier boundary.
    /// Used for tier hierarchy constraint (e.g., words must align with phones).
    /// @param tierIndex The tier index of the boundary being dragged.
    /// @param proposedTime The proposed new time (in microseconds).
    /// @param snapThreshold Maximum distance (in microseconds) for snapping.
    /// @return The snapped time, or the original time if no snap target found.
    [[nodiscard]] virtual TimePos snapToLowerTier(int /*tierIndex*/, TimePos proposedTime, TimePos /*snapThreshold*/) const {
        return proposedTime;
    }
};

} // namespace phonemelabeler
} // namespace dstools
