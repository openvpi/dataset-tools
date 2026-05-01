#pragma once

#include <QString>
#include <dstools/Result.h>
#include <functional>
#include <vector>

namespace dstools {

struct AlignedPhone {
    QString phone;
    double startSec;
    double endSec;
};

struct AlignmentResult {
    std::vector<AlignedPhone> phones;
    int sampleRate = 0;
};

struct AlignCsvOptions {
    std::vector<QString> uvVocab = {"AP", "SP", "br", "sil"};
    bool useWordBoundary = true;
};

class IAlignmentService {
public:
    virtual ~IAlignmentService() = default;

    virtual Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) = 0;
    virtual bool isModelLoaded() const = 0;
    virtual void unloadModel() = 0;

    virtual Result<AlignmentResult> align(const QString &audioPath,
                                          const std::vector<QString> &phonemes) = 0;

    virtual Result<void> alignCSV(const QString &csvPath, const QString &savePath,
                                  const AlignCsvOptions &options = {},
                                  const std::function<void(int)> &progress = nullptr) = 0;

    virtual nlohmann::json vocabInfo() const = 0;
};

} // namespace dstools
