#pragma once

#include <QString>
#include <vector>

struct AudioData {
    std::vector<double> samples;
    int sampleRate = 0;
    int channels = 0;
    qint64 frames = 0;

    qint64 totalSize() const { return frames * channels; }
};

struct AudioLoadResult {
    bool success = false;
    QString errorMessage;
    AudioData audio;
    QString sndfilePath;  // path usable with sndfile (original or temp WAV)
    QString tempFilePath; // non-empty if a temp WAV was created (caller must clean up)
};

class AudioFileLoader {
public:
    static AudioLoadResult load(const QString &filePath);
};
