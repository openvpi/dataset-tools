#pragma once

/// @file IAsrService.h
/// @brief Speech recognition service interface.

#include <QString>

#include <dstools/Result.h>

#include <vector>

namespace dstools {

/// @brief A single recognized segment with timing.
struct AsrSegment {
    QString text;    ///< Recognized text for this segment.
    double startSec; ///< Start time in seconds.
    double endSec;   ///< End time in seconds.
};

/// @brief Result of a speech recognition operation.
struct AsrResult {
    QString text;                    ///< Full recognized text.
    std::vector<AsrSegment> segments; ///< Time-aligned segments.
};

/// @brief Abstract interface for speech recognition backends.
class IAsrService {
public:
    virtual ~IAsrService() = default;

    /// @brief Recognize speech in an audio file.
    /// @param audioPath Path to the input audio file.
    /// @return AsrResult on success, or an error description.
    virtual Result<AsrResult> recognize(const QString &audioPath) = 0;
};

} // namespace dstools
