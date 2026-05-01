#pragma once

#include <QString>
#include <dstools/Result.h>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

namespace dstools {

struct NoteEvent {
    int pitch;
    int64_t startFrame;
    int64_t endFrame;
    float velocity;
};

struct TranscriptionResult {
    std::vector<NoteEvent> notes;
    int sampleRate = 0;
};

struct MidiNote {
    int note;
    int start;
    int duration;
};

struct TranscriptionModelInfo {
    int targetSampleRate = 0;
    float timestep = 0.01f;
    bool hasDur2bd = false;
    std::map<QString, int> languageMap;
};

class ITranscriptionService {
public:
    virtual ~ITranscriptionService() = default;

    virtual Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual void unloadModel() = 0;

    virtual Result<TranscriptionResult> transcribe(const QString &audioPath) = 0;

    virtual Result<void> exportMidi(const QString &audioPath, const QString &outputPath,
                                    float tempo,
                                    const std::function<void(int)> &progress = nullptr) = 0;

    virtual Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                                  const std::function<void(int)> &progress = nullptr) = 0;

    virtual TranscriptionModelInfo modelInfo() const = 0;
};

} // namespace dstools
