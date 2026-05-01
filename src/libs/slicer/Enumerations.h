#ifndef AUDIO_SLICER_CONSTANTS_H
#define AUDIO_SLICER_CONSTANTS_H

/// @file Enumerations.h
/// @brief Audio slicer enumeration types.

#include <QMetaType>

/// @brief PCM sample formats for audio I/O.
enum WaveFormat {
    WF_INT16_PCM,  ///< 16-bit signed integer PCM
    WF_INT24_PCM,  ///< 24-bit signed integer PCM
    WF_INT32_PCM,  ///< 32-bit signed integer PCM
    WF_FLOAT32     ///< 32-bit floating point
};
Q_DECLARE_METATYPE(WaveFormat)

/// @brief Slicing output modes controlling audio and marker generation.
enum class SlicingMode {
    AudioOnly,            ///< Output sliced audio files only
    AudioOnlyLoadMarkers, ///< Output sliced audio and load existing markers
    MarkersOnly,          ///< Output marker data only
    AudioAndMarkers       ///< Output both sliced audio and markers
};
Q_DECLARE_METATYPE(SlicingMode)

#endif // AUDIO_SLICER_CONSTANTS_H
