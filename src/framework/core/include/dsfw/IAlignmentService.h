#pragma once

/// @file IAlignmentService.h
/// @brief Forced alignment service interface.

#include <QString>

#include <dstools/Result.h>

#include <vector>

namespace dstools {

/// @brief A single aligned phoneme with timing information.
struct AlignedPhone {
    QString phone;   ///< Phoneme label.
    double startSec; ///< Start time in seconds.
    double endSec;   ///< End time in seconds.
};

/// @brief Result of a forced alignment operation.
struct AlignmentResult {
    std::vector<AlignedPhone> phones; ///< Aligned phoneme sequence.
    int sampleRate = 0;               ///< Sample rate of the source audio.
};

/// @brief Abstract interface for forced alignment backends (e.g. HuBERT).
class IAlignmentService {
public:
    virtual ~IAlignmentService() = default;

    /// @brief Align phonemes to an audio file.
    /// @param audioPath Path to the input audio file.
    /// @param phonemes Expected phoneme sequence to align.
    /// @return AlignmentResult on success, or an error description.
    virtual Result<AlignmentResult> align(const QString &audioPath,
                                         const std::vector<QString> &phonemes) = 0;
};

} // namespace dstools
