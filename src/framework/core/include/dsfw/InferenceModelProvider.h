#pragma once

/// @file InferenceModelProvider.h
/// @brief Template base class for ONNX Runtime inference model providers.

#include <dsfw/IModelProvider.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/IInferenceEngine.h>

namespace dstools {

using infer::ExecutionProvider;

/// @brief CRTP-style template that adapts an inference engine to the IModelProvider interface,
///        handling model loading with execution provider selection (CPU/DML/CUDA).
/// @tparam Engine The inference engine type (must provide load(), unload(), estimatedMemoryBytes()).
template <typename Engine>
class InferenceModelProvider : public IModelProvider {
public:
    /// @brief Construct a model provider with a type identifier and display name.
    /// @param typeId Unique model type identifier.
    /// @param name Human-readable model name.
    InferenceModelProvider(ModelTypeId typeId, const QString &name)
        : m_typeId(typeId), m_displayName(name), m_status(ModelStatus::Unloaded) {}

    /// @brief Access the underlying inference engine.
    /// @return Mutable reference to the engine.
    Engine &engine() { return m_engine; }

    /// @brief Access the underlying inference engine (const).
    /// @return Const reference to the engine.
    const Engine &engine() const { return m_engine; }

    /// @brief Return the model type identifier.
    ModelTypeId type() const override { return m_typeId; }

    /// @brief Return the human-readable model name.
    QString displayName() const override { return m_displayName; }

    /// @brief Load the model from disk with GPU selection.
    /// @param modelPath Path to the ONNX model file.
    /// @param gpuIndex GPU device index, or negative for CPU.
    /// @return Ok on success, Err with description on failure.
    Result<void> load(const QString &modelPath, int gpuIndex) override {
        m_status = ModelStatus::Loading;
        auto provider = gpuIndex < 0 ? ExecutionProvider::CPU : defaultGpuProvider();
        auto result = m_engine.load(modelPath.toStdWString(), provider, gpuIndex);
        if (!result) {
            m_status = ModelStatus::Error;
            return Err(result.error());
        }
        m_status = ModelStatus::Ready;
        return Ok();
    }

    /// @brief Release the loaded model and reset status to Unloaded.
    void unload() override {
        m_engine.unload();
        m_status = ModelStatus::Unloaded;
    }

    /// @brief Return the current model load state.
    ModelStatus status() const override { return m_status; }

    /// @brief Return the estimated memory footprint of the loaded model in bytes.
    int64_t estimatedMemoryBytes() const override {
        return m_engine.estimatedMemoryBytes();
    }

    /// @brief Return the platform-default GPU execution provider (DML on Windows, CUDA if enabled, else CPU).
    static ExecutionProvider defaultGpuProvider() {
#ifdef ONNXRUNTIME_ENABLE_DML
        return ExecutionProvider::DML;
#elif defined(ONNXRUNTIME_ENABLE_CUDA)
        return ExecutionProvider::CUDA;
#else
        return ExecutionProvider::CPU;
#endif
    }

private:
    Engine m_engine;              ///< Underlying inference engine instance.
    ModelTypeId m_typeId;         ///< Model type identifier.
    QString m_displayName;        ///< Human-readable model name.
    ModelStatus m_status;         ///< Current load state.
};

} // namespace dstools
