#pragma once

#include <QString>
#include <QStringList>
#include <dstools/DsProject.h>
#include <vector>

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

    class SliceExportService {
    public:
        static SliceExportResult exportSlices(const std::vector<float> &samples, int sampleRate,
                                              const std::vector<double> &slicePoints,
                                              const SliceExportOptions &options);
    };

} // namespace dstools