#ifndef RMVPE_H
#define RMVPE_H

#include <filesystem>
#include <functional>

#include <audio-util/SndfileVio.h>
#include <dstools/IInferenceEngine.h>
#include <dstools/Result.h>
#include <rmvpe-infer/Provider.h>
#include <rmvpe-infer/RmvpeGlobal.h>

namespace Rmvpe
{
    class RmvpeModel;

    struct RmvpeRes {
        float offset;
        std::vector<float> f0;
        std::vector<bool> uv;
    };

    class RMVPE_INFER_EXPORT Rmvpe : public dstools::infer::IInferenceEngine {
    public:
        Rmvpe();
        explicit Rmvpe(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~Rmvpe() override;

        bool is_open() const;
        bool isOpen() const override { return is_open(); }

        dstools::Result<void> get_f0(const std::filesystem::path &filepath, float threshold, std::vector<RmvpeRes> &res,
                                     const std::function<void(int)> &progressChanged) const;

        void terminate() override;

        const char *engineName() const override { return "RMVPE"; }

        dstools::Result<void> load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId) override;
        void unload() override;
        int64_t estimatedMemoryBytes() const override;

    private:
        std::unique_ptr<RmvpeModel> m_rmvpe;
    };
} // namespace Rmvpe

#endif // RMVPE_H
