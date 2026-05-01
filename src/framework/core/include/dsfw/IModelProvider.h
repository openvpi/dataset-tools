#pragma once

/// @file IModelProvider.h
/// @brief Abstract interface for loading and managing ML inference models.

#include <QString>

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include <dstools/Result.h>

namespace dstools {

/// @brief Extensible model type identifier (replaces fixed enum).
class ModelTypeId {
public:
    constexpr ModelTypeId() : m_id(-1) {}
    constexpr explicit ModelTypeId(int id) : m_id(id) {}
    constexpr int id() const { return m_id; }
    constexpr bool isValid() const { return m_id >= 0; }
    constexpr bool operator==(const ModelTypeId &o) const { return m_id == o.m_id; }
    constexpr bool operator!=(const ModelTypeId &o) const { return m_id != o.m_id; }
    constexpr bool operator<(const ModelTypeId &o) const { return m_id < o.m_id; }
private:
    int m_id;
};

/// @brief Register (or look up) a named model type, returning a stable ID.
/// @note Thread-safe: concurrent calls are serialized via an internal mutex.
inline ModelTypeId registerModelType(const std::string &name) {
    static std::mutex s_mutex;
    std::lock_guard<std::mutex> lock(s_mutex);
    static int s_nextId = 0;
    static std::unordered_map<std::string, int> s_registry;
    auto it = s_registry.find(name);
    if (it != s_registry.end())
        return ModelTypeId(it->second);
    int id = s_nextId++;
    s_registry[name] = id;
    return ModelTypeId(id);
}

/// @brief Lifecycle status of a model.
enum class ModelStatus {
    Unloaded, ///< Model is not loaded.
    Loading,  ///< Model is currently being loaded.
    Ready,    ///< Model is loaded and ready for inference.
    Error     ///< Model failed to load.
};

/// @brief Callback for model loading progress updates.
/// @param current Bytes or steps completed.
/// @param total Total bytes or steps.
/// @param msg Human-readable status message.
using ModelProgressCallback =
    std::function<void(int64_t current, int64_t total, const QString &msg)>;

/// @brief Abstract interface for a single ML model's lifecycle.
class IModelProvider {
public:
    virtual ~IModelProvider() = default;
    /// @brief Return the model type.
    /// @return ModelTypeId value.
    virtual ModelTypeId type() const = 0;
    /// @brief Return a human-readable name for this model.
    /// @return Display name string.
    virtual QString displayName() const = 0;
    /// @brief Load the model from the given path.
    /// @param modelPath Path to the model directory or file.
    /// @param gpuIndex GPU device index (-1 for CPU).
    /// @return Success or error.
    virtual Result<void> load(const QString &modelPath, int gpuIndex) = 0;
    /// @brief Unload the model and free resources.
    virtual void unload() = 0;
    /// @brief Return the current model status.
    /// @return ModelStatus enum value.
    virtual ModelStatus status() const = 0;
    /// @brief Estimate the model's memory consumption in bytes.
    /// @return Estimated bytes, or 0 if unknown.
    virtual int64_t estimatedMemoryBytes() const = 0;
    /// @brief Convenience check for Ready status.
    /// @return True if status() == ModelStatus::Ready.
    bool isReady() const { return status() == ModelStatus::Ready; }
};

}
