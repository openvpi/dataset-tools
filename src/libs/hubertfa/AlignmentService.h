#pragma once

#include <dsfw/IAlignmentService.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace HFA {
class HFA;
}

class AlignmentService : public dstools::IAlignmentService {
public:
    AlignmentService() = default;
    ~AlignmentService() override;

    dstools::Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;
    bool isModelLoaded() const override;
    void unloadModel() override;

    void setLanguage(const std::string &language);
    void setNonSpeechPhonemes(const std::vector<std::string> &phonemes);

    dstools::Result<dstools::AlignmentResult> align(const QString &audioPath,
                                            const std::vector<QString> &phonemes) override;

    dstools::Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                          const dstools::AlignCsvOptions &options = {},
                          const std::function<void(int)> &progress = nullptr) override;

    nlohmann::json vocabInfo() const override;

private:
    std::unique_ptr<HFA::HFA> m_hfa;
    std::string m_language = "zh";
    std::vector<std::string> m_nonSpeechPh;
    mutable std::mutex m_mutex;
};
