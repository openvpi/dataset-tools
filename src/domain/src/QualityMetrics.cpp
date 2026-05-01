#include <dstools/QualityMetrics.h>
#include <dsfw/JsonHelper.h>

#include <filesystem>

namespace dstools {

    const char *QualityMetrics::metricsName() const { return "Phoneme Duration Metrics"; }

    Result<ItemQualityReport> QualityMetrics::evaluate(const std::filesystem::path &sourceFile,
                                                        const std::filesystem::path &workingDir) {
        ItemQualityReport report;
        report.sourceFile = sourceFile.string();

        auto jsonResult = JsonHelper::loadFile(sourceFile);
        if (!jsonResult)
            return Err<ItemQualityReport>(jsonResult.error());

        const auto &j = jsonResult.value();
        (void)workingDir;

        try {
            if (j.contains("phonemes") && j["phonemes"].is_array()) {
                report.phonemeCount = static_cast<int>(j["phonemes"].size());
            }
            if (j.contains("duration") && j["duration"].is_number()) {
                report.durationSec = j["duration"].get<float>();
            }
        } catch (const std::exception &e) {
            return Err<ItemQualityReport>(std::string("Failed to evaluate quality: ") + e.what());
        }

        return Ok(std::move(report));
    }

    Result<std::vector<ItemQualityReport>> QualityMetrics::evaluateBatch(const std::filesystem::path &workingDir) {
        std::vector<ItemQualityReport> reports;

        try {
            for (const auto &entry : std::filesystem::directory_iterator(workingDir)) {
                if (entry.path().extension() == ".dsitem") {
                    auto report = evaluate(entry.path(), workingDir);
                    if (!report)
                        continue;
                    reports.push_back(std::move(report.value()));
                }
            }
        } catch (const std::exception &e) {
            return Err<std::vector<ItemQualityReport>>(std::string("Failed to scan working directory: ") + e.what());
        }

        return Ok(std::move(reports));
    }

} // namespace dstools
