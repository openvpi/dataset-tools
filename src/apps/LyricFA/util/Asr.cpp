#include "Asr.h"

#include <QBuffer>
#include <QFile>
#include <QMessageBox>
#include <sndfile.hh>

#include <Audio.h>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>

namespace LyricFA {

    Asr::Asr(const QString &modelPath) {
        m_asrHandle = std::unique_ptr<FunAsr::Model>(FunAsr::create_model(modelPath.toUtf8().toStdString().c_str(), 4));

        if (!m_asrHandle) {
            qDebug() << "Cannot load ASR Model, there must be files model.onnx and vocab.txt";
        }
    }

    Asr::~Asr() = default;

    bool Asr::recognize(const std::filesystem::path &filepath, std::string &msg) const {
        if (!m_asrHandle) {
            return false;
        }
        auto sf_vio = AudioUtil::resample_to_vio(filepath, msg, 1, 16000);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const AudioUtil::Slicer slicer(160, 0.02f, 160, 160 * 4, 500, 30, 50);
        const auto chunks = slicer.slice(audio);

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        int idx = 0;

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
                bytesWritten > 60 * 16000) {
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
            idx++;
        }


        if (msg.empty()) {
            msg = "Asr fail.";
            return false;
        }
        return true;
    }
} // LyricFA