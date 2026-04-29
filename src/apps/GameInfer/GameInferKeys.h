#pragma once
#include <dstools/AppSettings.h>

/// GameInfer settings key schema -- all persisted keys in one place.
namespace GameInferKeys {
    // Model
    inline const dstools::SettingsKey<QString> ModelPath("Model/path", "");

    // Processing parameters
    inline const dstools::SettingsKey<double> SegThreshold("Processing/segThreshold", 0.2);
    inline const dstools::SettingsKey<int>    SegRadiusFrame("Processing/segRadiusFrame", 2);
    inline const dstools::SettingsKey<double> EstThreshold("Processing/estThreshold", 0.2);
    inline const dstools::SettingsKey<int>    SegD3PMNSteps("Processing/segD3PMNStepsIndex", 3);
    inline const dstools::SettingsKey<int>    MaxAudioSegLength("Processing/maxAudioSegLength", 60);

    // Audio paths
    inline const dstools::SettingsKey<QString> WavPath("Audio/inputPath", "");
    inline const dstools::SettingsKey<QString> OutputMidiPath("Audio/outputMidiPath", "");

    // Align CSV paths
    inline const dstools::SettingsKey<QString> AlignCsvInputPath("Align/csvInputPath", "");
    inline const dstools::SettingsKey<QString> AlignWavDir("Align/wavDir", "");
    inline const dstools::SettingsKey<QString> AlignOutputPath("Align/outputPath", "");
}
