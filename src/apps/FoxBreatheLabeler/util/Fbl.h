#ifndef ASR_H
#define ASR_H

#include <memory>

#include <QString>

#include "FblModel.h"
#include <filesystem>

namespace FBL {

    class FBL {
    public:
        explicit FBL(const QString &modelDir);
        ~FBL();

        bool recognize(const std::filesystem::path &filepath, std::vector<std::pair<float, float>> &res,
                       std::string &msg, float ap_threshold = 0.4, float ap_dur = 0.08) const;

    private:
        std::unique_ptr<FblModel> m_fblModel;

        int m_audio_sample_rate;
        int m_hop_size;

        float m_time_scale;
    };
} // LyricFA

#endif // ASR_H
