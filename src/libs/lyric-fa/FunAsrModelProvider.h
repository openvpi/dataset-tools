#pragma once

/// @file FunAsrModelProvider.h
/// @brief IModelProvider implementation wrapping LyricFA::Asr.

#include <dsfw/IModelProvider.h>

#include <memory>

namespace LyricFA {
class Asr;
}

namespace dstools {

class FunAsrModelProvider : public IModelProvider {
public:
    FunAsrModelProvider(ModelTypeId typeId, const QString &name);
    ~FunAsrModelProvider() override;

    FunAsrModelProvider(const FunAsrModelProvider &) = delete;
    FunAsrModelProvider &operator=(const FunAsrModelProvider &) = delete;

    LyricFA::Asr *asr() const;

    ModelTypeId type() const override;
    QString displayName() const override;
    Result<void> load(const QString &modelPath, int gpuIndex) override;
    void unload() override;
    ModelStatus status() const override;
    int64_t estimatedMemoryBytes() const override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr;
    ModelTypeId m_typeId;
    QString m_displayName;
    ModelStatus m_status;
};

} // namespace dstools
