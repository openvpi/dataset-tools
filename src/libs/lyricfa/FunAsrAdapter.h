#pragma once

#include <dstools/IInferenceEngine.h>

#include <memory>
#include <mutex>

namespace FunAsr {
    class Model;
}

namespace LyricFA {

    class FunAsrAdapter : public dstools::infer::IInferenceEngine {
    public:
        FunAsrAdapter() = default;
        ~FunAsrAdapter() override;

        FunAsrAdapter(const FunAsrAdapter &) = delete;
        FunAsrAdapter &operator=(const FunAsrAdapter &) = delete;

        bool isOpen() const override;
        const char *engineName() const override;
        dstools::Result<void> load(const std::filesystem::path &modelPath,
                                   dstools::infer::ExecutionProvider provider, int deviceId) override;
        bool load(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId,
                  std::string &errorMsg);
        void unload() override;

        FunAsr::Model *model() const;

    private:
        std::unique_ptr<FunAsr::Model> m_model;
        mutable std::mutex m_mutex;
    };

} // namespace LyricFA
