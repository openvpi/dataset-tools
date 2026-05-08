#include "AsrPipeline.h"

#include <Model.h>

#include <QApplication>
#include <QBuffer>
#include <QFile>
#include <QMessageBox>
#include <QTextStream>
#include <sndfile.hh>

#include <Audio.h>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>

#include <dsfw/TaskProcessorRegistry.h>

namespace LyricFA {

// ─── Asr ──────────────────────────────────────────────────────────────────────

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

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_FLOAT, kAsrChannels, kAsrSampleRate);
        if (!sf) {
            msg = "Failed to open resampled audio for ASR";
            return false;
        }
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

// ─── AsrTask ──────────────────────────────────────────────────────────────────

    AsrThread::AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
                         const QSharedPointer<Pinyin::Pinyin> &g2p)
        : AsyncTask(std::move(filename)), m_asr(asr), m_wavPath(std::move(wavPath)), m_labPath(std::move(labPath)),
          m_g2p(g2p) {
    }

    bool AsrThread::execute(QString &msg) {
        std::string asrMsg;

        if (const auto asrRes = m_asr->recognize(m_wavPath.toLocal8Bit().toStdString(), asrMsg); !asrRes) {
            msg = QString::fromStdString(asrMsg);
            return false;
        }

        QFile labFile(m_labPath);
        if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            msg = QString("Failed to write to file %1").arg(m_labPath);
            return false;
        }

        QTextStream labIn(&labFile);
        if (m_g2p != nullptr) {
            const auto g2pRes = m_g2p->hanziToPinyin(asrMsg, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, true);
            asrMsg = g2pRes.toStdStr();
        }

        labIn << QString::fromStdString(asrMsg);
        labFile.close();
        msg = QString::fromStdString(asrMsg);
        return true;
    }

// ─── FunAsrAdapter ────────────────────────────────────────────────────────────

FunAsrAdapter::~FunAsrAdapter() = default;

bool FunAsrAdapter::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model != nullptr;
}

const char *FunAsrAdapter::engineName() const {
    return "FunASR";
}

dstools::Result<void> FunAsrAdapter::load(const std::filesystem::path &modelPath,
                                            dstools::infer::ExecutionProvider provider, int deviceId) {
    std::string errorMsg;
    if (!load(modelPath, provider, deviceId, errorMsg))
        return dstools::Result<void>::Error(errorMsg);
    return dstools::Result<void>::Ok();
}

bool FunAsrAdapter::load(const std::filesystem::path &modelPath,
                          dstools::infer::ExecutionProvider provider, int deviceId,
                          std::string &errorMsg) {
    unload();

    auto *raw = FunAsr::create_model(modelPath, 4, provider, deviceId);
    if (!raw) {
        errorMsg = "Failed to load FunASR model from: " + modelPath.string();
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_model.reset(raw);
    return true;
}

void FunAsrAdapter::unload() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_model.reset();
}

FunAsr::Model *FunAsrAdapter::model() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model.get();
}

} // namespace LyricFA

// ─── FunAsrProcessor ──────────────────────────────────────────────────────────

namespace dstools {

static TaskProcessorRegistry::Registrar<FunAsrProcessor> s_reg(
    QStringLiteral("asr"), QStringLiteral("funasr"));

FunAsrProcessor::FunAsrProcessor() = default;
FunAsrProcessor::~FunAsrProcessor() = default;

QString FunAsrProcessor::processorId() const {
    return QStringLiteral("funasr");
}

QString FunAsrProcessor::displayName() const {
    return QStringLiteral("FunASR Speech Recognition");
}

TaskSpec FunAsrProcessor::taskSpec() const {
    return {QStringLiteral("asr"), {}, {{QStringLiteral("text"), QStringLiteral("transcription")}}};
}

Result<void> FunAsrProcessor::initialize(IModelManager & /*mm*/,
                                         const ProcessorConfig &modelConfig) {
    std::lock_guard lock(m_mutex);

    const auto path = QString::fromStdString(modelConfig.value("path", ""));
    const int deviceId = modelConfig.value("deviceId", -1);

    auto provider = deviceId < 0 ? FunAsr::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : FunAsr::ExecutionProvider::DML;
#else
                                 : FunAsr::ExecutionProvider::CPU;
#endif

    m_asr = std::make_unique<LyricFA::Asr>(path, provider, deviceId);
    if (!m_asr->initialized()) {
        m_asr.reset();
        return Result<void>::Error("Failed to initialize FunASR model");
    }
    return Result<void>::Ok();
}

void FunAsrProcessor::release() {
    std::lock_guard lock(m_mutex);
    m_asr.reset();
}

Result<TaskOutput> FunAsrProcessor::process(const TaskInput &input) {
    std::lock_guard lock(m_mutex);

    if (!m_asr || !m_asr->initialized()) {
        return Result<TaskOutput>::Error("FunASR model is not initialized");
    }

    std::string msg;
    const bool ok = m_asr->recognize(input.audioPath.toLocal8Bit().toStdString(), msg);
    if (!ok) {
        return Result<TaskOutput>::Error(msg);
    }

    TaskOutput output;
    output.layers[QStringLiteral("text")] = nlohmann::json{{u8"text", msg}};
    return Result<TaskOutput>::Ok(std::move(output));
}

} // namespace dstools
