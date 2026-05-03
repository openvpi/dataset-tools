#pragma once

#include <dstools/DsProject.h>

#include <QString>
#include <QStringList>
#include <vector>

namespace AudioUtil {
struct SlicerParams;
}

namespace dstools {

struct SliceExportOptions {
    QString outputDir;
    QString prefix;
    int numDigits = 3;
    int bitDepth = 16;
    int channels = 1;
    std::vector<int> discardedIndices;
};

struct SliceExportResult {
    int exportedCount = 0;
    QStringList errors;
    std::vector<Item> items;
};

class SlicerService {
public:
    static std::vector<double> computeSlicePoints(const std::vector<float> &samples, int sampleRate,
                                                   const AudioUtil::SlicerParams &params);

    static std::vector<float> loadAndMixAudio(const QString &filePath, int *outSampleRate);

    static SliceExportResult exportSlices(const std::vector<float> &samples, int sampleRate,
                                          const std::vector<double> &slicePoints,
                                          const SliceExportOptions &options);
};

} // namespace dstools
