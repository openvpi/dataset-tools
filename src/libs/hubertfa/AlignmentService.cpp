#include "AlignmentService.h"

#include <hubert-infer/Hfa.h>

#include <dstools/ExecutionProvider.h>

AlignmentService::~AlignmentService() = default;

dstools::Result<void> AlignmentService::loadModel(const QString &modelPath, int gpuIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto provider = gpuIndex < 0 ? dstools::infer::ExecutionProvider::CPU
#ifdef ONNXRUNTIME_ENABLE_DML
                                 : dstools::infer::ExecutionProvider::DML;
#else
                                 : dstools::infer::ExecutionProvider::CPU;
#endif

    m_hfa = std::make_unique<HFA::HFA>();
    auto result = m_hfa->load(modelPath.toStdWString(), provider, gpuIndex);
    if (!result) {
        m_hfa.reset();
        return dstools::Err(result.error());
    }
    return dstools::Ok();
}

bool AlignmentService::isModelLoaded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_hfa && m_hfa->isOpen();
}

void AlignmentService::unloadModel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hfa.reset();
}

void AlignmentService::setLanguage(const std::string &language) {
    m_language = language;
}

void AlignmentService::setNonSpeechPhonemes(const std::vector<std::string> &phonemes) {
    m_nonSpeechPh = phonemes;
}

dstools::Result<dstools::AlignmentResult> AlignmentService::align(const QString &audioPath,
                                                          const std::vector<QString> &phonemes) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_hfa || !m_hfa->isOpen()) {
        return dstools::Result<dstools::AlignmentResult>::Error("Alignment model is not loaded");
    }

    HFA::WordList words;
    for (const auto &ph : phonemes) {
        HFA::Word word;
        word.text = ph.toStdString();
        HFA::Phoneme p;
        p.text = ph.toStdString();
        word.phones.push_back(p);
        words.push_back(word);
    }

    auto recognizeResult = m_hfa->recognize(audioPath.toLocal8Bit().toStdString(), m_language, m_nonSpeechPh, words);
    if (!recognizeResult) {
        return dstools::Result<dstools::AlignmentResult>::Error(recognizeResult.error());
    }

    dstools::AlignmentResult result;
    for (const auto &word : words) {
        for (const auto &phone : word.phones) {
            dstools::AlignedPhone ap;
            ap.phone = QString::fromStdString(phone.text);
            ap.startSec = phone.start;
            ap.endSec = phone.end;
            result.phones.push_back(std::move(ap));
        }
    }

    return dstools::Result<dstools::AlignmentResult>::Ok(std::move(result));
}

dstools::Result<void> AlignmentService::alignCSV(const QString &csvPath, const QString &savePath,
                                          const dstools::AlignCsvOptions &options,
                                          const std::function<void(int)> &progress) {
    (void)csvPath;
    (void)savePath;
    (void)options;
    (void)progress;
    return dstools::Err("CSV alignment not supported by HFA alignment service");
}

nlohmann::json AlignmentService::vocabInfo() const {
    return nlohmann::json::object();
}
