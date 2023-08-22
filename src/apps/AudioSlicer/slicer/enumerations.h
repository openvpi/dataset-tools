#ifndef AUDIO_SLICER_CONSTANTS_H
#define AUDIO_SLICER_CONSTANTS_H

#include <QMetaType>

enum WaveFormat {
    WF_INT16_PCM,
    WF_INT24_PCM,
    WF_INT32_PCM,
    WF_FLOAT32
};
Q_DECLARE_METATYPE(WaveFormat)

enum class SlicingMode {
    AudioOnly,
    AudioOnlyLoadMarkers,
    MarkersOnly,
    AudioAndMarkers
};
Q_DECLARE_METATYPE(SlicingMode)

#endif // AUDIO_SLICER_CONSTANTS_H