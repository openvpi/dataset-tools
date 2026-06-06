#include <moe-infer/Moe.h>
#include <dsfw/PathUtils.h>
#include "MoeModel.h"

namespace Moe
{
    Moe::Moe() = default;

    Moe::Moe(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId) {
        load(modelPath, provider, deviceId);
    }

    Moe::~Moe() {
        unload();
    }

    bool Moe::isOpen() const {
        return m_model && m_model->is_open();
    }

    void Moe::terminate() {
        if (m_model) {
            m_model->terminate();
        }
    }

    dstools::Result<void> Moe::load(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId) {
        unload();
        m_model = std::make_unique<MoeModel>(modelPath, provider, deviceId);
        if (!m_model->is_open()) {
            m_model.reset();
            return dstools::Result<void>::Error("Failed to load MOE model: " + dsfw::PathUtils::toUtf8(modelPath));
        }
        return dstools::Result<void>::Ok();
    }

    void Moe::unload() {
        m_model.reset();
    }

    int64_t Moe::estimatedMemoryBytes() const {
        return 50 * 1024 * 1024;
    }

    dstools::Result<MoeResult> Moe::predict(const std::vector<float> &waveform) const {
        if (!m_model || !m_model->is_open()) {
            return dstools::Result<MoeResult>::Error("MOE model is not loaded.");
        }

        auto result = m_model->predict(waveform.data(), static_cast<int64_t>(waveform.size()));
        if (!result) {
            return dstools::Result<MoeResult>::Error(result.error());
        }

        MoeResult moeResult;
        moeResult.curve = std::move(result.value());
        return dstools::Result<MoeResult>::Ok(std::move(moeResult));
    }

} // namespace Moe