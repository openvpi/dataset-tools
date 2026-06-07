#pragma once

/// @file AsrPipeline.h
/// @brief ASR call chain: FunAsrAdapter → FunAsrProcessor → AsrThread → Asr

#include <QSharedPointer>
#include <QString>
#include <cpp-pinyin/Pinyin.h>
#include <dsfw/AsyncTask.h>
#include <dsfw/ITaskProcessor.h>
#include <dsfw/infer/IInferenceEngine.h>
#include <filesystem>
#include <memory>
#include <mutex>

namespace FunAsr {
class Model;
using ExecutionProvider = dsfw::infer::ExecutionProvider;
}  // namespace FunAsr

namespace LyricFA {

class Asr {
public:
    explicit Asr(const QString& modelPath,
                 FunAsr::ExecutionProvider provider = FunAsr::ExecutionProvider::CPU,
                 int deviceId = 0);
    ~Asr();

    bool recognize(const std::filesystem::path& filepath, std::string& msg) const;

    bool initialized() const {
        return m_asrHandle != nullptr;
    }

private:
    std::unique_ptr<FunAsr::Model> m_asrHandle;
    mutable std::mutex m_mutex;
};

class ILyricFileLoader;

class AsrThread final : public dsfw::AsyncTask {
    Q_OBJECT
public:
    AsrThread(Asr* asr,
              QString filename,
              QString wavPath,
              QString labPath,
              const QSharedPointer<Pinyin::Pinyin>& g2p,
              ILyricFileLoader* loader = nullptr);

protected:
    bool execute(QString& msg) override;

private:
    Asr* m_asr;
    QString m_wavPath;
    QString m_labPath;
    QSharedPointer<Pinyin::Pinyin> m_g2p = nullptr;
    ILyricFileLoader* m_loader = nullptr;
};

class FunAsrAdapter : public dsfw::infer::IInferenceEngine {
public:
    FunAsrAdapter() = default;
    ~FunAsrAdapter() override;

    FunAsrAdapter(const FunAsrAdapter&) = delete;
    FunAsrAdapter& operator=(const FunAsrAdapter&) = delete;

    bool isOpen() const override;
    const char* engineName() const override;

    dsfw::Result<void>
    load(const std::filesystem::path& modelPath, dsfw::infer::ExecutionProvider provider, int deviceId) override;

    bool load(const std::filesystem::path& modelPath,
              dsfw::infer::ExecutionProvider provider,
              int deviceId,
              std::string& errorMsg);

    void unload() override;
    FunAsr::Model* model() const;

private:
    std::unique_ptr<FunAsr::Model> m_model;
    mutable std::mutex m_mutex;
};

}  // namespace LyricFA

namespace dstools {

class FunAsrProcessor : public dsfw::ITaskProcessor {
public:
    FunAsrProcessor();
    ~FunAsrProcessor() override;

    QString processorId() const override;
    QString displayName() const override;
    dsfw::TaskSpec taskSpec() const override;

    dsfw::Result<void> initialize(ModelManager& mm, const dsfw::ProcessorConfig& modelConfig) override;
    void release() override;
    dsfw::Result<dsfw::TaskOutput> process(const dsfw::TaskInput& input) override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr;
    mutable std::mutex m_mutex;
};

}  // namespace dstools
