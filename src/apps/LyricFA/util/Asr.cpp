#include "Asr.h"

#include <QBuffer>
#include <QDebug>
#include <QFile>

#include <CDSPResampler.h>
#include <sndfile.hh>

namespace LyricFA {
    sf_count_t qvio_get_filelen(void *user_data) {
        const auto *qvio = static_cast<QVIO *>(user_data);
        return qvio->byteArray.size();
    }

    sf_count_t qvio_seek(sf_count_t offset, int whence, void *user_data) {
        auto *qvio = static_cast<QVIO *>(user_data);
        switch (whence) {
            case SEEK_SET:
                qvio->seek = offset;
                break;
            case SEEK_CUR:
                qvio->seek += offset;
                break;
            case SEEK_END:
                qvio->seek = qvio->byteArray.size() + offset;
                break;
            default:
                break;
        }
        return qvio->seek;
    }

    sf_count_t qvio_read(void *ptr, sf_count_t count, void *user_data) {
        auto *qvio = static_cast<QVIO *>(user_data);
        const sf_count_t remainingBytes = qvio->byteArray.size() - qvio->seek;
        const sf_count_t bytesToRead = min(count, remainingBytes);
        if (bytesToRead > 0) {
            std::memcpy(ptr, qvio->byteArray.constData() + qvio->seek, bytesToRead);
            qvio->seek += bytesToRead;
        }
        return bytesToRead;
    }

    sf_count_t qvio_write(const void *ptr, sf_count_t count, void *user_data) {
        auto *qvio = static_cast<QVIO *>(user_data);
        auto *data = static_cast<const char *>(ptr);
        qvio->byteArray.append(data, static_cast<int>(count));
        return count;
    }

    sf_count_t qvio_tell(void *user_data) {
        const auto *qvio = static_cast<QVIO *>(user_data);
        return qvio->seek;
    }


    Asr::Asr(const QString &modelPath) {
        m_asrHandle = RapidAsrInit(modelPath.toStdString().c_str(), 4);

        if (!m_asrHandle) {
            printf("Cannot load ASR Model, there must be files model.onnx and vocab.txt");
            exit(-1);
        }
    }

    Asr::~Asr() {
        RapidAsrUninit(m_asrHandle);
    }

    QString Asr::recognize(const QVIO &qvio) const {
        QString msg;
        if (const auto Result = RapidAsrRecogPCMBuffer(m_asrHandle, qvio.byteArray.constData(), qvio.byteArray.size(),
                                                       RASR_NONE, nullptr)) {
            msg = RapidAsrGetResult(Result, 0);
            RapidAsrFreeResult(Result);
        } else {
            qDebug() << ("no return data!");
        }
        return msg;
    }

    QString Asr::recognize(const QString &filename) const {
        return recognize(resample(filename));
    }

    QVIO Asr::resample(const QString &filename) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open file:" << file.errorString();
            return {};
        }

        // 读取WAV文件头信息
        SndfileHandle srcHandle(file.fileName().toLocal8Bit(), SFM_READ);
        if (!srcHandle) {
            qDebug() << "Failed to open WAV file:" << sf_strerror(nullptr);
            return {};
        }

        // 临时文件
        QVIO qvio;
        SF_VIRTUAL_IO virtual_io;
        virtual_io.get_filelen = qvio_get_filelen;
        virtual_io.seek = qvio_seek;
        virtual_io.read = qvio_read;
        virtual_io.write = qvio_write;
        virtual_io.tell = qvio_tell;

        SndfileHandle outBuf(virtual_io, &qvio, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1, 16000);
        if (!outBuf) {
            qDebug() << "Failed to open output file:" << sf_strerror(nullptr);
            return {};
        }

        // 创建 CDSPResampler 对象
        r8b::CDSPResampler resampler(srcHandle.samplerate(), 16000, srcHandle.samplerate());

        // 重采样并写入输出文件
        double *op0;
        std::vector<double> tmp(srcHandle.samplerate() * srcHandle.channels());
        qlonglong total = 0;

        // 逐块读取、重采样并写入输出文件
        while (true) {
            const auto bytesRead =
                srcHandle.read(tmp.data(), static_cast<sf_count_t>(tmp.size())) / srcHandle.channels();
            if (bytesRead <= 0) {
                break; // 读取结束
            }

            // 处理重采样
            const int outSamples = resampler.process(tmp.data(), static_cast<int>(bytesRead), op0);
            // 写入输出文件
            const auto bytesWritten = outBuf.write(op0, outSamples);

            if (bytesWritten != outSamples) {
                qDebug() << "Error writing to output file";
                break;
            }

            total += bytesWritten;

            // 如果读取的数据不足一个块大小，则跳出循环
            if (bytesRead < srcHandle.samplerate()) {
                break;
            }
        }

        qDebug() << "FileName: " + filename +
                        " Seconds: " + QString::number(static_cast<double>(total)/16000,'f',2)+ "s";

        return qvio;
    }
} // LyricFA