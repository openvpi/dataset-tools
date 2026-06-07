#pragma once

/// @file FunAsrModelProvider.h
/// @brief IModelProvider implementation wrapping LyricFA::Asr.

#include <dsfw/IModelProvider.h>
#include <memory>

namespace LyricFA {
class Asr;
}

namespace dstools {

class FunAsrModelProvider : public dsfw::IModelProvider {
public:
    FunAsrModelProvider(dsfw::ModelTypeId typeId, const QString& name);
    ~FunAsrModelProvider() override;

    FunAsrModelProvider(const FunAsrModelProvider&) = delete;
    FunAsrModelProvider& operator=(const FunAsrModelProvider&) = delete;

    LyricFA::Asr* asr() const;

    LyricFA::Asr& engine() {
        return *m_asr;
    }
    const LyricFA::Asr& engine() const {
        return *m_asr;
    }

    dsfw::ModelTypeId type() const override;
    QString displayName() const override;
    dsfw::Result<void> load(const QString& modelPath, int gpuIndex) override;
    void unload() override;
    dsfw::ModelStatus status() const override;
    int64_t estimatedMemoryBytes() const override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr;
    dsfw::ModelTypeId m_typeId;
    QString m_displayName;
    dsfw::ModelStatus m_status;
};

}  // namespace dstools
