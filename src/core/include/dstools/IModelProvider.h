#pragma once

#include <QString>

#include <cstdint>
#include <functional>
#include <string>

namespace dstools {

enum class ModelType {
    Asr,        // FunASR speech recognition
    HuBERT,     // HuBERT phoneme alignment
    GAME,       // GAME audio-to-MIDI
    RMVPE,      // RMVPE F0 estimation
    SOME,       // SOME model
    Custom      // User-defined
};

enum class ModelStatus {
    Unloaded,
    Loading,
    Ready,
    Error
};

using ModelProgressCallback =
    std::function<void(int64_t current, int64_t total, const QString &msg)>;

class IModelProvider {
public:
    virtual ~IModelProvider() = default;
    virtual ModelType type() const = 0;
    virtual QString displayName() const = 0;
    virtual bool load(const QString &modelPath, int gpuIndex, std::string &error) = 0;
    virtual void unload() = 0;
    virtual ModelStatus status() const = 0;
    virtual int64_t estimatedMemoryBytes() const = 0;
    bool isReady() const { return status() == ModelStatus::Ready; }
};

} // namespace dstools
