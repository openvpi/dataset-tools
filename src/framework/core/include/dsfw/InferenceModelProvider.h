#pragma once

#include <dsfw/IModelProvider.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/IInferenceEngine.h>

namespace dstools {

template <typename Engine>
class InferenceModelProvider : public IModelProvider {
public:
    InferenceModelProvider(ModelTypeId typeId, const QString &name)
        : m_typeId(typeId), m_displayName(name), m_status(ModelStatus::Unloaded) {}

    Engine &engine() { return m_engine; }
    const Engine &engine() const { return m_engine; }

    ModelTypeId type() const override { return m_typeId; }
    QString displayName() const override { return m_displayName; }

    Result<void> load(const QString &modelPath, int gpuIndex) override {
        m_status = ModelStatus::Loading;
        auto provider = gpuIndex < 0 ? ExecutionProvider::CPU : defaultGpuProvider();
        auto result = m_engine.load(modelPath.toStdWString(), provider, gpuIndex);
        if (!result) {
            m_status = ModelStatus::Error;
            return Err(QString::fromStdString(result.error()));
        }
        m_status = ModelStatus::Ready;
        return Ok();
    }

    void unload() override {
        m_engine.unload();
        m_status = ModelStatus::Unloaded;
    }

    ModelStatus status() const override { return m_status; }

    int64_t estimatedMemoryBytes() const override {
        return m_engine.estimatedMemoryBytes();
    }

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
    Engine m_engine;
    ModelTypeId m_typeId;
    QString m_displayName;
    ModelStatus m_status;
};

} // namespace dstools
