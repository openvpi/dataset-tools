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

    bool loadModel(const QString &modelPath, int gpuIndex = -1);
    bool isModelLoaded() const;
    void unloadModel();

    void setLanguage(const std::string &language);
    void setNonSpeechPhonemes(const std::vector<std::string> &phonemes);

    Result<dstools::AlignmentResult> align(const QString &audioPath,
                                           const std::vector<QString> &phonemes) override;

private:
    std::unique_ptr<HFA::HFA> m_hfa;
    std::string m_language = "zh";
    std::vector<std::string> m_nonSpeechPh;
    mutable std::mutex m_mutex;
};
