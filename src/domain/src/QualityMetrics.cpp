#include <dstools/QualityMetrics.h>

#include <sndfile.h>

#include <QDir>
#include <QFileInfo>

#include <cmath>
#include <numeric>

namespace dstools {

QString QualityMetrics::providerName() const {
    return QStringLiteral("audio-quality");
}

QStringList QualityMetrics::availableMetrics() const {
    return {QStringLiteral("Duration (s)"), QStringLiteral("SNR (dB)")};
}

ItemQualityReport QualityMetrics::evaluate(const QString &sourceFile, const QString &workingDir,
                                           std::string &error) const {
    ItemQualityReport report;
    report.sourceFile = sourceFile;

    const QString path = QDir(workingDir).filePath(sourceFile);
    SF_INFO sfInfo{};
    SNDFILE *file = sf_open(path.toStdString().c_str(), SFM_READ, &sfInfo);
    if (!file) {
        error = sf_strerror(nullptr);
        return report;
    }

    // Duration
    const double duration = static_cast<double>(sfInfo.frames) / static_cast<double>(sfInfo.samplerate);

    // Read samples for SNR estimation
    std::vector<float> samples(static_cast<size_t>(sfInfo.frames * sfInfo.channels));
    const sf_count_t readCount = sf_readf_float(file, samples.data(), sfInfo.frames);
    sf_close(file);

    if (readCount <= 0) {
        error = "Failed to read audio samples";
        return report;
    }

    // Mix to mono if multi-channel
    std::vector<float> mono(static_cast<size_t>(readCount));
    for (sf_count_t i = 0; i < readCount; ++i) {
        float sum = 0.0f;
        for (int ch = 0; ch < sfInfo.channels; ++ch) {
            sum += samples[static_cast<size_t>(i * sfInfo.channels + ch)];
        }
        mono[static_cast<size_t>(i)] = sum / static_cast<float>(sfInfo.channels);
    }

    // Estimate SNR: ratio of RMS signal to RMS of noise floor (bottom 10% amplitude frames)
    double totalEnergy = 0.0;
    for (const float s : mono) {
        totalEnergy += static_cast<double>(s) * static_cast<double>(s);
    }
    const double rmsSignal = std::sqrt(totalEnergy / static_cast<double>(mono.size()));

    // Sort absolute values to find noise floor (lowest 10%)
    std::vector<float> absValues(mono.size());
    for (size_t i = 0; i < mono.size(); ++i) {
        absValues[i] = std::fabs(mono[i]);
    }
    std::sort(absValues.begin(), absValues.end());

    const size_t noiseCount = std::max<size_t>(1, mono.size() / 10);
    double noiseEnergy = 0.0;
    for (size_t i = 0; i < noiseCount; ++i) {
        noiseEnergy += static_cast<double>(absValues[i]) * static_cast<double>(absValues[i]);
    }
    const double rmsNoise = std::sqrt(noiseEnergy / static_cast<double>(noiseCount));

    double snr = 0.0;
    if (rmsNoise > 1e-10) {
        snr = 20.0 * std::log10(rmsSignal / rmsNoise);
    }

    // Build report
    report.metrics.push_back({QStringLiteral("Duration (s)"), duration, QStringLiteral("s"), {}});
    report.metrics.push_back({QStringLiteral("SNR (dB)"), snr, QStringLiteral("dB"), {}});
    report.overallScore = snr; // Use SNR as the primary quality indicator

    error.clear();
    return report;
}

std::vector<ItemQualityReport> QualityMetrics::evaluateBatch(const QString &workingDir,
                                                             std::string &error) const {
    std::vector<ItemQualityReport> reports;
    const QDir dir(workingDir);
    const QStringList filters = {
        QStringLiteral("*.wav"), QStringLiteral("*.flac"), QStringLiteral("*.ogg"),
        QStringLiteral("*.aiff"), QStringLiteral("*.mp3")};

    const auto entries = dir.entryList(filters, QDir::Files);
    for (const QString &entry : entries) {
        std::string itemError;
        auto report = evaluate(entry, workingDir, itemError);
        if (itemError.empty()) {
            reports.push_back(std::move(report));
        }
    }

    error.clear();
    return reports;
}

double QualityMetrics::aggregateScore(const std::vector<ItemQualityReport> &reports) const {
    if (reports.empty())
        return 0.0;

    // Weighted average: weight by duration so longer files contribute more
    double weightedSum = 0.0;
    double totalWeight = 0.0;

    for (const auto &report : reports) {
        double duration = 1.0; // default weight
        for (const auto &m : report.metrics) {
            if (m.metricName == QStringLiteral("Duration (s)")) {
                duration = std::max(m.value, 0.01);
                break;
            }
        }
        weightedSum += report.overallScore * duration;
        totalWeight += duration;
    }

    return (totalWeight > 0.0) ? (weightedSum / totalWeight) : 0.0;
}

} // namespace dstools
