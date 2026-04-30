#include "Asr.h"

#include <QBuffer>
#include <QFile>
#include <QMessageBox>
#include <sndfile.hh>

#include <Audio.h>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>

namespace LyricFA {

    constexpr int kAsrSampleRate = 16000;
    constexpr int kAsrChannels = 1;
    constexpr int kSlicerWindowSize = 160;
    constexpr float kSlicerThreshold = 0.02f;
    constexpr int kSlicerMinLength = 160;
    constexpr int kSlicerMinInterval = 160 * 4;
    constexpr int kSlicerHopSize = 500;
    constexpr int kSlicerMaxSilKept = 30;
    constexpr int kSlicerMinSilKept = 50;
    constexpr int kMaxChunkDurationSec = 60;

    Asr::Asr(const QString &modelPath, FunAsr::ExecutionProvider provider, int deviceId) {
        m_asrHandle = std::unique_ptr<FunAsr::Model>(
            FunAsr::create_model(modelPath.toUtf8().toStdString().c_str(), 4, provider, deviceId));

        if (!m_asrHandle) {
            qDebug() << "Cannot load ASR Model, there must be files model.onnx and vocab.txt";
        }
    }

    Asr::~Asr() = default;

    bool Asr::recognize(const std::filesystem::path &filepath, std::string &msg) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_asrHandle) {
            return false;
        }
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, kAsrChannels, kAsrSampleRate);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, kAsrChannels, kAsrSampleRate);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(kSlicerWindowSize, kSlicerThreshold, kSlicerMinLength, kSlicerMinInterval, kSlicerHopSize, kSlicerMaxSilKept, kSlicerMinSilKept);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        for (const auto &[fst, snd] : chunks) {
            const auto beginFrame = fst;
            const auto endFrame = snd;
            const auto frameCount = endFrame - beginFrame;
            if (frameCount <= 0 || beginFrame > totalSize || endFrame > totalSize) {
                continue;
            }

            sf.seek(beginFrame, SEEK_SET);
            std::vector<float> tmp(frameCount);

            if (const auto bytesWritten = sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));
                bytesWritten > kMaxChunkDurationSec * kAsrSampleRate) {
                msg = "The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
                      "segment and rerun the recognition program.";
                return false;
            }

            FunAsr::Audio asr_audio(1);
            asr_audio.loadPcmFloat(tmp.data(), static_cast<int>(tmp.size()));

            float *buff;
            int len;
            int flag = 0;
            while (asr_audio.fetch(buff, len, flag) > 0) {
                m_asrHandle->reset();
                msg += m_asrHandle->forward(buff, len, flag);
            }
        }


        if (msg.empty()) {
            msg = "Asr fail.";
            return false;
        }
        return true;
    }
} // LyricFA