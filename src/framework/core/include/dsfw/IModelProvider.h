#pragma once

#include <QString>

#include <cstdint>
#include <functional>
#include <string>

#include <dstools/Result.h>

namespace dstools {

enum class ModelType {
    Asr,
    HuBERT,
    GAME,
    RMVPE,
    SOME,
    Custom
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
    virtual Result<void> load(const QString &modelPath, int gpuIndex) = 0;
    virtual void unload() = 0;
    virtual ModelStatus status() const = 0;
    virtual int64_t estimatedMemoryBytes() const = 0;
    bool isReady() const { return status() == ModelStatus::Ready; }
};

}
