#ifndef RMVPE_H
#define RMVPE_H

#include <filesystem>
#include <functional>

#include <audio-util/SndfileVio.h>
#include <rmvpe-infer/Provider.h>
#include <rmvpe-infer/RmvpeGlobal.h>

namespace Rmvpe
{
    class RmvpeModel;

    struct RmvpeRes {
        float offset; // ms
        std::vector<float> f0;
        std::vector<bool> uv;
    };

    class RMVPE_INFER_EXPORT Rmvpe {
    public:
        explicit Rmvpe(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~Rmvpe();

        bool is_open() const;

        bool get_f0(const std::filesystem::path &filepath, float threshold, std::vector<RmvpeRes> &res,
                    std::string &msg, const std::function<void(int)> &progressChanged) const;

        void terminate() const;

        std::unique_ptr<RmvpeModel> m_rmvpe;
    };
} // namespace Rmvpe

#endif // RMVPE_H
