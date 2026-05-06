#include <rmvpe-infer/Rmvpe.h>

#include <sndfile.hh>

#include <iostream>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>
#include <rmvpe-infer/RmvpeModel.h>

namespace Rmvpe
{
    Rmvpe::Rmvpe() = default;

    Rmvpe::Rmvpe(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id) {
        load(modelPath, provider, device_id);
    }

    Rmvpe::~Rmvpe() = default;

    bool Rmvpe::is_open() const { return m_rmvpe && m_rmvpe->is_open(); }

    static float calculateSumOfDifferences(const AudioUtil::MarkerList &markers) {
        float sum = 0;
        for (const auto &[fst, snd] : markers) {
            sum += static_cast<float>(snd - fst);
        }
        return sum;
    }

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

    dstools::Result<void> Rmvpe::get_f0(const std::filesystem::path &filepath, const float threshold, std::vector<RmvpeRes> &res,
                                          const std::function<void(int)> &progressChanged) const {
        if (!m_rmvpe) {
            return dstools::Err("RMVPE model not loaded");
        }

        std::string msg;
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, 16000);

        if (!msg.empty()) {
            return dstools::Err("Failed to resample audio for RMVPE: " + msg);
        }

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
        if (!sf) {
            return dstools::Err("Failed to open resampled audio for RMVPE");
        }
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(160, 0.02f, 160, 160 * 4, 500, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            return dstools::Err("slicer: no audio chunks for output!");
        }


        int processedFrames = 0;
        const float slicerFrames = calculateSumOfDifferences(chunks);

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;
            if (frameCount <= 0 || beginFrame > totalSize || endFrame > totalSize) {
                continue;
            }

            sf.seek(static_cast<sf_count_t>(beginFrame), SEEK_SET);
            std::vector<float> tmp(frameCount);
            sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));

            RmvpeRes tempRes;
            tempRes.offset = static_cast<float>(static_cast<double>(fst) / (16000.0 / 1000));
            const bool success = m_rmvpe->forward(tmp, threshold, tempRes.f0, tempRes.uv, msg);
            if (!success)
                return dstools::Err(msg);
            interp_f0(tempRes.f0, tempRes.uv);
            res.push_back(tempRes);

            processedFrames += static_cast<int>(frameCount);
            int progress = static_cast<int>((static_cast<float>(processedFrames) / slicerFrames) * 100);

            if (progressChanged) {
                progressChanged(progress);
            }
        }
        return dstools::Ok();
    }

    void Rmvpe::terminate() { m_rmvpe->terminate(); }

    dstools::Result<void> Rmvpe::load(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int deviceId) {
        unload();
        try {
            m_rmvpe = std::make_unique<RmvpeModel>(modelPath, provider, deviceId);
            if (!m_rmvpe->is_open()) {
                m_rmvpe.reset();
                return dstools::Err("Cannot load RMVPE Model from " + modelPath.string());
            }
            return dstools::Ok();
        } catch (const std::exception &e) {
            m_rmvpe.reset();
            return dstools::Err(e.what());
        }
    }

    void Rmvpe::unload() {
        m_rmvpe.reset();
    }

    int64_t Rmvpe::estimatedMemoryBytes() const {
        return is_open() ? 100 * 1024 * 1024LL : 0;
    }

} // namespace Rmvpe
