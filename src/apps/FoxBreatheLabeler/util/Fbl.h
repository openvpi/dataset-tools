#ifndef ASR_H
#define ASR_H

#include <memory>

#include <QString>

#include "FblModel.h"

#include "SndfileVio.h"

namespace FBL {

    class FBL {
    public:
        explicit FBL(const QString &modelDir);
        ~FBL();

        [[nodiscard]] bool recognize(const QString &filename, std::vector<std::pair<float, float>> &res, QString &msg, float ap_threshold = 0.4,
                                     float ap_dur = 0.1) const;
        [[nodiscard]] bool recognize(SF_VIO sf_vio, std::vector<std::pair<float, float>> &res, QString &msg, float ap_threshold = 0.4, float ap_dur = 0.08) const;

    private:
        [[nodiscard]] SF_VIO resample(const QString &filename) const;

        std::unique_ptr<FblModel> m_fblModel;

        int m_audio_sample_rate;
        int m_spec_win;
        int m_hop_size;

        float m_time_scale;
    };
} // LyricFA

#endif // ASR_H
