#include <dstools/DsItemManager.h>
#include <dsfw/JsonHelper.h>

#include <filesystem>

namespace dstools {

    Result<void> DsItemManager::load(const std::filesystem::path &dsitemPath, DsItemRecord &record) {
        auto jsonResult = JsonHelper::loadFile(dsitemPath);
        if (!jsonResult)
            return Err(jsonResult.error());

        const auto &j = jsonResult.value();
        try {
            record.status = j.value("status", DsItemRecord::Status::Pending);
            record.inputFile = j.value("inputFile", "");
            record.outputFile = j.value("outputFile", "");
            record.errorMsg = j.value("errorMsg", "");
            record.timestamp = j.value("timestamp", "");
            return Ok();
        } catch (const std::exception &e) {
            return Err(std::string("Failed to parse dsitem: ") + e.what());
        }
    }

    Result<void> DsItemManager::save(const DsItemRecord &record, const std::filesystem::path &dsitemPath) {
        nlohmann::json j;
        j["status"] = record.status;
        j["inputFile"] = record.inputFile;
        j["outputFile"] = record.outputFile;
        j["errorMsg"] = record.errorMsg;
        j["timestamp"] = record.timestamp;
        return JsonHelper::saveFile(dsitemPath, j);
    }

    Result<DsItemRecord::Status> DsItemManager::queryStatus(const std::filesystem::path &dsitemPath) {
        DsItemRecord record;
        auto loadResult = load(dsitemPath, record);
        if (!loadResult)
            return Err(loadResult.error());
        return Ok(record.status);
    }

    Result<bool> DsItemManager::needsProcessing(const std::filesystem::path &dsitemPath) {
        DsItemRecord record;
        auto loadResult = load(dsitemPath, record);
        if (!loadResult)
            return Err(loadResult.error());
        return Ok(record.status == DsItemRecord::Status::Pending);
    }

    Result<void> DsItemManager::markCompleted(const std::filesystem::path &dsitemPath) {
        DsItemRecord record;
        auto loadResult = load(dsitemPath, record);
        if (!loadResult)
            return Err(loadResult.error());
        record.status = DsItemRecord::Status::Completed;
        record.timestamp = ""; // TODO: add timestamp
        return save(record, dsitemPath);
    }

    Result<void> DsItemManager::markFailed(const std::filesystem::path &dsitemPath, const std::string &errorMsg) {
        DsItemRecord record;
        auto loadResult = load(dsitemPath, record);
        if (!loadResult)
            return Err(loadResult.error());
        record.status = DsItemRecord::Status::Failed;
        record.errorMsg = errorMsg;
        record.timestamp = ""; // TODO: add timestamp
        return save(record, dsitemPath);
    }

    Result<DsItemManager::StepSummary> DsItemManager::summarizeStep(const std::filesystem::path &workingDir) {
        StepSummary summary;
        try {
            for (const auto &entry : std::filesystem::directory_iterator(workingDir)) {
                if (entry.path().extension() == ".dsitem") {
                    DsItemRecord record;
                    auto loadResult = load(entry.path(), record);
                    if (!loadResult)
                        continue;
                    summary.total++;
                    switch (record.status) {
                        case DsItemRecord::Status::Completed:
                            summary.completed++;
                            break;
                        case DsItemRecord::Status::Failed:
                            summary.failed++;
                            break;
                        case DsItemRecord::Status::Pending:
                        default:
                            summary.pending++;
                            break;
                    }
                }
            }
        } catch (const std::exception &e) {
            return Err(std::string("Failed to scan working directory: ") + e.what());
        }
        return Ok(summary);
    }

} // namespace dstools
