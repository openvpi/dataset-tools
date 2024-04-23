#include "Asr.h"

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QMessageBox>

#include <CDSPResampler.h>
#include <sndfile.hh>

#include <Audio.h>

#include "Slicer.h"

namespace LyricFA {

    Asr::Asr(const QString &modelPath) {
        m_asrHandle = FunAsr::create_model(modelPath.toLocal8Bit().toStdString().c_str(), 4);

        if (!m_asrHandle) {
            qDebug() << "Cannot load ASR Model, there must be files model.onnx and vocab.txt";
        }
    }

    Asr::~Asr() {
        delete m_asrHandle;
    }

    bool Asr::recognize(SF_VIO sf_vio, QString &msg) const {
        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
        Slicer slicer(&sf, -40, 5000, 300, 10, 1000);

        const auto chunks = slicer.slice();

        if (chunks.empty()) {
            msg = "slicer: no audio chunks for output!";
            return false;
        }

        const auto frames = sf.frames();
        const auto totalSize = frames;

        int idx = 0;
        for (const auto &chunk : chunks) {
            const auto beginFrame = chunk.first;
            const auto endFrame = chunk.second;
            const auto frameCount = endFrame - beginFrame;
            if ((frameCount <= 0) || (beginFrame > totalSize) || (endFrame > totalSize) || (beginFrame < 0) ||
                (endFrame < 0)) {
                continue;
            }

            SF_VIO sfChunk;
            auto wf = SndfileHandle(sfChunk.vio, &sfChunk.data, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
            sf.seek(beginFrame, SEEK_SET);
            std::vector<double> tmp(frameCount);
            sf.read(tmp.data(), static_cast<sf_count_t>(tmp.size()));
            const auto bytesWritten = wf.write(tmp.data(), static_cast<sf_count_t>(tmp.size()));

            if (bytesWritten > 60 * 16000) {
                msg = "The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
                      "segment and rerun the recognition program.";
                return false;
            }

            FunAsr::Audio audio(1);
            audio.loadwav(sfChunk.constData(), sfChunk.size());

            float *buff;
            int len;
            int flag = 0;
            while (audio.fetch(buff, len, flag) > 0) {
                m_asrHandle->reset();
                msg += QString::fromStdString(m_asrHandle->forward(buff, len, flag));
            }
            idx++;
        }

        if (msg.isEmpty()) {
            msg = "Asr fail.";
            return false;
        }
        return true;
    }

    bool Asr::recognize(const QString &filename, QString &msg) const {
        return recognize(resample(filename), msg);
    }

    SF_VIO Asr::resample(const QString &filename) {
        // 读取WAV文件头信息
        SndfileHandle srcHandle(filename.toLocal8Bit(), SFM_READ, SF_FORMAT_WAV);
        if (!srcHandle) {
            qDebug() << "Failed to open WAV file:" << sf_strerror(nullptr);
            return {};
        }

        // 临时文件
        SF_VIO sf_vio;
        SndfileHandle outBuf(sf_vio.vio, &sf_vio.data, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
        if (!outBuf) {
            qDebug() << "Failed to open output file:" << sf_strerror(nullptr);
            return {};
        }

        // 创建 CDSPResampler 对象
        r8b::CDSPResampler16 resampler(srcHandle.samplerate(), 16000, srcHandle.samplerate());

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
                                                     static_cast<double>(srcHandle.samplerate()) * 16000.0 -
                                                 total)) {
            std::vector<double> inputBuf(tmp.size() / srcHandle.channels());
            resampler.process(inputBuf.data(), srcHandle.samplerate(), op0);
            outBuf.write(op0, endSize);
        }

        return sf_vio;
    }
} // LyricFA