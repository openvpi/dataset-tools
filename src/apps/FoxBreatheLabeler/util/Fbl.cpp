#include "Fbl.h"

#include <QBuffer>
#include <QDebug>
#include <QMessageBox>

#include <CDSPResampler.h>
#include <QDir>
#include <sndfile.hh>

#include <cmath>
#include <stdexcept>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace FBL {

    FBL::FBL(const QString &modelDir) {
        const auto modelPath = modelDir + QDir::separator() + "model.onnx";
        const auto configPath = modelDir + QDir::separator() + "config.yaml";
        m_fblModel = std::make_unique<FblModel>(FblModel(modelPath.toUtf8().toStdString()));

        YAML::Node config = YAML::LoadFile(configPath.toUtf8().toStdString());

        // 获取参数
        m_audio_sample_rate = config["audio_sample_rate"].as<int>();
        m_spec_win = config["spec_win"].as<int>();
        m_hop_size = config["hop_size"].as<int>();

        m_time_scale = 1.0 / (m_audio_sample_rate / m_hop_size);

        if (!m_fblModel) {
            qDebug() << "Cannot load ASR Model, there must be files model.onnx and vocab.txt";
        }
    }

    FBL::~FBL() = default;

    // Function to pad the signal
    std::vector<float> pad_signal(const std::vector<float> &y, const std::pair<int, int> &padding,
                                  const std::string &pad_mode = "constant") {
        if (pad_mode == "constant") {
            const int left_pad = padding.first;
            const int right_pad = padding.second;
            std::vector<float> y_padded(left_pad + y.size() + right_pad, 0.0);
            std::copy(y.begin(), y.end(), y_padded.begin() + left_pad);
            return y_padded;
        } else {
            throw std::invalid_argument("Unsupported pad_mode");
        }
    }

    // Function to unfold the signal into frames
    std::vector<std::vector<float>> unfold_signal(const std::vector<float> &y, int frame_length, int hop_length) {
        const int num_frames = (static_cast<int>(y.size()) - frame_length) / hop_length + 1;
        std::vector<std::vector<float>> unfolded;
        for (int i = 0; i < num_frames; ++i) {
            const int start = i * hop_length;
            const int end = start + frame_length;
            unfolded.emplace_back(y.begin() + start, y.begin() + end);
        }
        return unfolded;
    }

    // Function to get music chunks
    std::vector<std::vector<float>> get_music_chunk(const std::vector<float> &y, int frame_length = 2048,
                                                    int hop_length = 512, const std::string &pad_mode = "constant") {
        const std::pair<int, int> padding = {(frame_length - hop_length) / 2, (frame_length - hop_length + 1) / 2};
        const std::vector<float> y_padded = pad_signal(y, padding, pad_mode);
        std::vector<std::vector<float>> y_f = unfold_signal(y_padded, frame_length, hop_length);

        return y_f;
    }

    std::vector<float> sigmoid(const std::vector<float> &arr) {
        std::vector<float> result;
        result.reserve(arr.size());
        for (const auto &item : arr)
            result.push_back(1 / (1 + std::exp(-item)));
        return result;
    }

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

    bool FBL::recognize(SF_VIO sf_vio, std::vector<std::pair<float, float>> &res, QString &msg, float ap_threshold,
                        float ap_dur) const {
        if (!m_fblModel) {
            return false;
        }
        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, m_audio_sample_rate);

        const auto frames = sf.frames();

        SF_VIO sfChunk;
        sf.seek(0, SEEK_SET);
        std::vector<float> tmp(frames);
        sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));

        if (static_cast<double>(frames) / m_audio_sample_rate > 60) {
            msg = "The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
                  "segment and rerun the recognition program.";
            return false;
        }

        std::string modelMsg;
        std::vector<float> modelRes;
        if (m_fblModel->forward(get_music_chunk(tmp, m_spec_win, m_hop_size), modelRes, modelMsg)) {
            res = findSegmentsDynamic(sigmoid(modelRes), m_time_scale, ap_threshold,
                                      static_cast<int>(ap_dur / m_time_scale));
            return true;
        } else {
            msg = QString::fromStdString(modelMsg);
            return false;
        }
    }

    bool FBL::recognize(const QString &filename, std::vector<std::pair<float, float>> &res, QString &msg,
                        float ap_threshold, float ap_dur) const {
        return recognize(resample(filename), res, msg, ap_threshold, ap_dur);
    }

    SF_VIO FBL::resample(const QString &filename) const {
        // 读取WAV文件头信息
        SndfileHandle srcHandle(filename.toLocal8Bit(), SFM_READ, SF_FORMAT_WAV);
        if (!srcHandle) {
            qDebug() << "Failed to open WAV file:" << sf_strerror(nullptr);
            return {};
        }

        // 临时文件
        SF_VIO sf_vio;
        SndfileHandle outBuf(sf_vio.vio, &sf_vio.data, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1,
                             m_audio_sample_rate);
        if (!outBuf) {
            qDebug() << "Failed to open output file:" << sf_strerror(nullptr);
            return {};
        }

        // 创建 CDSPResampler 对象
        r8b::CDSPResampler16 resampler(srcHandle.samplerate(), m_audio_sample_rate, srcHandle.samplerate());

        // 重采样并写入输出文件
        double *op0;
        std::vector<double> tmp(srcHandle.samplerate() * srcHandle.channels());
        double total = 0;

        // 逐块读取、重采样并写入输出文件
        while (true) {
            const auto bytesRead = srcHandle.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));
            if (bytesRead <= 0) {
                break; // 读取结束
            }

            // 转单声道
            std::vector<double> inputBuf(tmp.size() / srcHandle.channels());
            for (int i = 0; i < tmp.size(); i += srcHandle.channels()) {
                inputBuf[i / srcHandle.channels()] = tmp[i];
            }

            // 处理重采样
            const int outSamples =
                resampler.process(inputBuf.data(), static_cast<int>(bytesRead) / srcHandle.channels(), op0);

            // 写入输出文件
            const auto bytesWritten = static_cast<double>(outBuf.write(op0, outSamples));

            if (bytesWritten != outSamples) {
                qDebug() << "Error writing to output file";
                break;
            }
            total += bytesWritten;
        }

        if (const int endSize = static_cast<int>(static_cast<double>(srcHandle.frames()) /
                                                     static_cast<double>(srcHandle.samplerate()) * m_audio_sample_rate -
                                                 total)) {
            std::vector<double> inputBuf(tmp.size() / srcHandle.channels());
            resampler.process(inputBuf.data(), srcHandle.samplerate(), op0);
            outBuf.write(op0, endSize);
        }

        return sf_vio;
    }
} // LyricFA