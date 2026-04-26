#include "SliceExportService.h"

#include <dsfw/PathUtils.h>

#include <QDir>

#include <sndfile.hh>

#include <algorithm>

namespace dstools {

SliceExportResult SliceExportService::exportSlices(const std::vector<float> &samples, int sampleRate,
                                                    const std::vector<double> &slicePoints,
                                                    const SliceExportOptions &options) {
    SliceExportResult result;

    if (slicePoints.empty() || samples.empty())
        return result;

    QDir dir(options.outputDir);
    if (!dir.exists())
        dir.mkpath(options.outputDir);

    int sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    switch (options.bitDepth) {
        case 16: sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16; break;
        case 24: sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_24; break;
        case 32: sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_32; break;
        default: sndFormat = SF_FORMAT_WAV | SF_FORMAT_FLOAT; break;
    }

    int numSegments = static_cast<int>(slicePoints.size()) + 1;
    for (int i = 0; i < numSegments; ++i) {
        double startSec = (i == 0) ? 0.0 : slicePoints[i - 1];
        double endSec = (i < static_cast<int>(slicePoints.size()))
                            ? slicePoints[i]
                            : static_cast<double>(samples.size()) / sampleRate;
        int startSamp = static_cast<int>(startSec * sampleRate);
        int endSamp = std::min(static_cast<int>(endSec * sampleRate), static_cast<int>(samples.size()));
        if (endSamp <= startSamp)
            continue;

        QString filename = QStringLiteral("%1_%2.wav").arg(options.prefix).arg(i + 1, options.numDigits, 10, QChar('0'));
        QString filepath = dir.filePath(filename);

#ifdef _WIN32
        auto pathStr = filepath.toStdWString();
#else
        auto pathStr = filepath.toStdString();
#endif
        SndfileHandle wf(pathStr.c_str(), SFM_WRITE, sndFormat, options.channels, sampleRate);
        if (!wf) {
            result.errors.append(filepath);
            continue;
        }

        sf_count_t frameCount = endSamp - startSamp;
        sf_count_t written = wf.write(samples.data() + startSamp, frameCount);
        if (written != frameCount) {
            result.errors.append(filepath);
            continue;
        }

        bool isDiscarded = std::find(options.discardedIndices.begin(), options.discardedIndices.end(), i) !=
                           options.discardedIndices.end();

        QString sliceId = QStringLiteral("%1_%2").arg(options.prefix).arg(i + 1, options.numDigits, 10, QChar('0'));

        Slice slice;
        slice.id = sliceId;
        slice.name = sliceId;
        slice.inPos = static_cast<int64_t>(startSec * 1000000.0);
        slice.outPos = static_cast<int64_t>(endSec * 1000000.0);
        slice.status = isDiscarded ? QStringLiteral("discarded") : QStringLiteral("active");

        Item item;
        item.id = sliceId;
        item.name = sliceId;
        item.audioSource = dsfw::PathUtils::toPosixSeparators(
            dir.filePath(QStringLiteral("%1_%2.wav").arg(options.prefix).arg(i + 1, options.numDigits, 10, QChar('0'))));
        item.slices.push_back(std::move(slice));
        result.items.push_back(std::move(item));

        ++result.exportedCount;
    }

    return result;
}

} // namespace dstools