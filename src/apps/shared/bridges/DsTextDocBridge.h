#pragma once

#include <dstools/DsPitchDocument.h>
#include <dstools/DsTextTypes.h>
#include <dstools/Result.h>
#include <dstools/TimePos.h>
#include <QList>
#include <memory>

namespace dstools {

/// @brief Bridge between DsTextDocument (internal model) and DsPitchDocument (pitch-labeler domain).
///
/// Provides bidirectional conversion and roundtrip verification between the
/// application's internal document model and the pitch-labeler's domain model.
/// All methods are static — this is a stateless utility class.
class DsTextDocBridge {
public:
    /// Extract all interval layers from a document (shallow copy).
    static QList<IntervalLayer> extractIntervalLayers(const DsTextDocument& doc);

    /// Merge edited interval layers back into a document.
    /// Existing layers are updated in-place; new layers are appended.
    static void mergeIntervalLayers(DsTextDocument& doc, const QList<IntervalLayer>& layers);

    /// Convert a DsTextDocument to a DsPitchDocument for pitch-labeler processing.
    /// Extracts F0 curve, MIDI notes, and phoneme boundaries.
    /// @param totalDurationUs  Total audio duration in microseconds (for computing phone durations).
    static std::shared_ptr<pitchlabeler::DsPitchDocument> toPitchDoc(const DsTextDocument& doc,
                                                                     TimePos totalDurationUs);

    /// Apply pitch-labeler results back into a DsTextDocument.
    /// Updates F0 curve, MIDI boundaries, and phoneme boundaries in-place.
    [[nodiscard]] static Result<void> fromPitchDoc(DsTextDocument& doc, const pitchlabeler::DsPitchDocument& pdoc);

    /// Verify that interval layers survive a roundtrip (original vs restored).
    /// Checks layer count, names, boundary count, and boundary positions.
    [[nodiscard]] static Result<void> verifyLayerRoundtrip(const DsTextDocument& original,
                                                           const DsTextDocument& restored);

    /// Verify that pitch document data survives a roundtrip.
    /// Checks F0 value count and MIDI boundary count.
    [[nodiscard]] static Result<void> verifyPitchDocRoundtrip(const DsTextDocument& original,
                                                              const pitchlabeler::DsPitchDocument& restored);
};

} // namespace dstools