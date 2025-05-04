#include "Fbl.h"

#include "audio-util/Slicer.h"

#include <QBuffer>
#include <QMessageBox>

#include <QDir>

#include <stdexcept>
#include <vector>

#include <audio-util/Util.h>
#include <yaml-cpp/yaml.h>

namespace FBL {

    FBL::FBL(const QString &modelDir) {
        const auto modelPath = modelDir + QDir::separator() + "model.onnx";
        const auto configPath = modelDir + QDir::separator() + "config.yaml";
        m_fblModel = std::make_unique<FblModel>(modelPath.toUtf8().toStdString());

        YAML::Node config = YAML::LoadFile(configPath.toUtf8().toStdString());

        // 获取参数
        m_audio_sample_rate = config["audio_sample_rate"].as<int>();
        m_hop_size = config["hop_size"].as<int>();

        m_time_scale = 1 / (static_cast<float>(m_audio_sample_rate) / static_cast<float>(m_hop_size));

        if (!m_fblModel) {
            qDebug() << "Cannot load ASR Model, there must be files model.onnx and vocab.txt";
        }
    }

    FBL::~FBL() = default;

    static std::vector<std::pair<float, float>> findSegmentsDynamic(const std::vector<float> &arr, double time_scale,
                                                                    double threshold = 0.5, int max_gap = 5,
                                                                    int ap_threshold = 10) {
        std::vector<std::pair<float, float>> segments;
        int start = -1;
        int gap_count = 0;

        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i] >= threshold) {
                if (start == -1) {
                    start = i;
                }
                gap_count = 0;
            } else {
                if (start != -1) {
                    if (gap_count < max_gap) {
                        gap_count++;
                    } else {
                        const int end = i - gap_count - 1;
                        if (end >= start && (end - start) >= ap_threshold) {
                            segments.emplace_back(start * time_scale, end * time_scale);
                        }
                        start = -1;
                        gap_count = 0;
                    }
                }
            }
        }

        // Handle the case where the array ends with a segment
        if (start != -1 && (arr.size() - start) >= ap_threshold) {
            segments.emplace_back(start * time_scale, (arr.size() - 1) * time_scale);
        }

        return segments;
    }

    bool FBL::recognize(const std::filesystem::path &filepath, std::vector<std::pair<float, float>> &res,
                        std::string &msg, float ap_threshold, float ap_dur) const {
        if (!m_fblModel) {
            return false;
        }

        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, 16000);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        if (static_cast<double>(sf.frames()) / m_audio_sample_rate > 60) {
            msg = "The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
                  "segment and rerun the recognition program.";
            return false;
        }

        std::string modelMsg;
        std::vector<float> modelRes;
        if (m_fblModel->forward(std::vector<std::vector<float>>{audio}, modelRes, modelMsg)) {
            res = findSegmentsDynamic(modelRes, m_time_scale, ap_threshold, 5, static_cast<int>(ap_dur / m_time_scale));
            return true;
        } else {
            msg = modelMsg;
            return false;
        }
    }
} // LyricFA