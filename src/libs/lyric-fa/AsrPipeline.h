#pragma once

/// @file AsrPipeline.h
/// @brief ASR call chain: FunAsrAdapter → FunAsrProcessor → AsrThread → Asr

#include <dsfw/AsyncTask.h>
#include <dsfw/ITaskProcessor.h>
#include <dstools/ExecutionProvider.h>
#include <dstools/IInferenceEngine.h>

#include <QSharedPointer>
#include <QString>
#include <cpp-pinyin/Pinyin.h>
#include <filesystem>
#include <memory>
#include <mutex>

namespace FunAsr {
class Model;
using ExecutionProvider = dstools::infer::ExecutionProvider;
} // namespace FunAsr

namespace LyricFA {

class Asr {
public:
    explicit Asr(const QString &modelPath,
                 FunAsr::ExecutionProvider provider = FunAsr::ExecutionProvider::CPU,
                 int deviceId = 0);
    ~Asr();

    bool recognize(const std::filesystem::path &filepath, std::string &msg) const;

    bool initialized() const {
        return m_asrHandle != nullptr;
    }

private:
    std::unique_ptr<FunAsr::Model> m_asrHandle;
    mutable std::mutex m_mutex;
};

class AsrThread final : public dstools::AsyncTask {
    Q_OBJECT
public:
    AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
              const QSharedPointer<Pinyin::Pinyin> &g2p);

protected:
    bool execute(QString &msg) override;

private:
    Asr *m_asr;
    QString m_wavPath;
    QString m_labPath;
    QSharedPointer<Pinyin::Pinyin> m_g2p = nullptr;
};

class FunAsrAdapter : public dstools::infer::IInferenceEngine {
public:
    FunAsrAdapter() = default;
    ~FunAsrAdapter() override;

    FunAsrAdapter(const FunAsrAdapter &) = delete;
    FunAsrAdapter &operator=(const FunAsrAdapter &) = delete;

    bool isOpen() const override;
    const char *engineName() const override;

    dstools::Result<void> load(const std::filesystem::path &modelPath,
                               dstools::infer::ExecutionProvider provider, int deviceId) override;

    bool load(const std::filesystem::path &modelPath, dstools::infer::ExecutionProvider provider, int deviceId,
              std::string &errorMsg);

    void unload() override;
    FunAsr::Model *model() const;

private:
    std::unique_ptr<FunAsr::Model> m_model;
    mutable std::mutex m_mutex;
};

} // namespace LyricFA

namespace dstools {

class FunAsrProcessor : public ITaskProcessor {
public:
    FunAsrProcessor();
    ~FunAsrProcessor() override;

    QString processorId() const override;
    QString displayName() const override;
    TaskSpec taskSpec() const override;

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &modelConfig) override;
    void release() override;
    Result<TaskOutput> process(const TaskInput &input) override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr;
    mutable std::mutex m_mutex;
};

} // namespace dstools
