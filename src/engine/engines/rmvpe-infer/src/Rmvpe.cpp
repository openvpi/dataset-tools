#include <rmvpe-infer/Rmvpe.h>
#include <dsfw/PathUtils.h>

#include <iostream>

#include <dsfw/audio/AudioPipeline.h>
#include <rmvpe-infer/RmvpeModel.h>

namespace Rmvpe
{
    Rmvpe::Rmvpe() = default;

    Rmvpe::Rmvpe(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) {
        load(modelPath, provider, device_id);
    }

    Rmvpe::~Rmvpe() = default;

    bool Rmvpe::is_open() const { return m_rmvpe && m_rmvpe->is_open(); }

    static void interp_f0(std::vector<float> &f0, std::vector<bool> &uv) {
        const int n = static_cast<int>(f0.size());

        bool all_voiced = true;
        for (int i = 0; i < n; ++i) {
            if (!uv[i]) {
                all_voiced = false;
                break;
            }
        }
        if (all_voiced) {
            return;
        }

        int first_true = -1;
        int last_true = -1;

        for (int i = 0; i < n; ++i) {
            if (!uv[i]) {
                first_true = i;
                break;
            }
        }

        for (int i = n - 1; i >= 0; --i) {
            if (!uv[i]) {
                last_true = i;
                break;
            }
        }

        if (first_true == -1 || last_true == -1) {
            return;
        }

        for (int i = 0; i < first_true; ++i) {
            f0[i] = f0[first_true];
        }

        for (int i = n - 1; i > last_true; --i) {
            f0[i] = f0[last_true];
        }

        for (int i = first_true; i < last_true; ++i) {
            if (uv[i]) {
                const int prev = i - 1;
                int next = i + 1;
                while (next < n && uv[next]) {
                    next++;
                }
                if (next < n) {
                    const float ratio = std::log(f0[next] / f0[prev]);
                    f0[i] = static_cast<float>(f0[prev] *
                                               std::exp(ratio * static_cast<long double>(i - prev) / (next - prev)));
                }
            }
        }
    }

    dsfw::Result<void> Rmvpe::get_f0(const std::filesystem::path &filepath, const float threshold, std::vector<RmvpeRes> &res,
                                          const std::function<void(int)> &progressChanged) const {
        if (!m_rmvpe) {
            return dsfw::Err("RMVPE model not loaded");
        }

        auto pipeline = dsfw::audio::AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(dsfw::PathUtils::toUtf8(filepath), 16000);
        if (!result.ok()) {
            return dsfw::Err("Failed to decode audio for RMVPE: " + result.error());
        }
        auto buffer = result.value();
        auto floats = buffer.floats();
        std::vector<float> audio(floats.begin(), floats.end());

        std::string msg;
        RmvpeRes tempRes;
        tempRes.offset = 0.0f;
        const bool success = m_rmvpe->forward(audio, threshold, tempRes.f0, tempRes.uv, msg);
        if (!success)
            return dsfw::Err(msg);
        interp_f0(tempRes.f0, tempRes.uv);
        res.push_back(tempRes);

        if (progressChanged) {
            progressChanged(100);
        }
        return dsfw::Ok();
    }

    void Rmvpe::terminate() { m_rmvpe->terminate(); }

    dsfw::Result<void> Rmvpe::load(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int deviceId) {
        unload();
        try {
            m_rmvpe = std::make_unique<RmvpeModel>(modelPath, provider, deviceId);
            if (!m_rmvpe->is_open()) {
                m_rmvpe.reset();
                return dsfw::Err("Cannot load RMVPE Model from " + dsfw::PathUtils::toUtf8(modelPath));
            }
            return dsfw::Ok();
        } catch (const std::exception &e) {
            m_rmvpe.reset();
            return dsfw::Err(e.what());
        }
    }

    void Rmvpe::unload() {
        m_rmvpe.reset();
    }

    int64_t Rmvpe::estimatedMemoryBytes() const {
        return is_open() ? 100 * 1024 * 1024LL : 0;
    }

} // namespace Rmvpe
