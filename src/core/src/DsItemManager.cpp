#include <dstools/DsItemManager.h>
#include <dstools/DsDocument.h>
#include <dsfw/JsonHelper.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace dstools {

// ── Construction ──────────────────────────────────────────────────────

DsItemManager::DsItemManager(const std::filesystem::path &projectRoot)
    : m_projectRoot(projectRoot) {
}

// ── String ↔ Enum mappings ───────────────────────────────────────────

QString DsItemManager::stepToString(PipelineStep step) {
    switch (step) {
        case PipelineStep::Slicer:          return QStringLiteral("slicer");
        case PipelineStep::Asr:             return QStringLiteral("asr");
        case PipelineStep::AsrReview:       return QStringLiteral("asr_review");
        case PipelineStep::Alignment:       return QStringLiteral("alignment");
        case PipelineStep::AlignmentReview: return QStringLiteral("alignment_review");
        case PipelineStep::BuildCsv:        return QStringLiteral("build_csv");
        case PipelineStep::Midi:            return QStringLiteral("midi");
        case PipelineStep::BuildDs:         return QStringLiteral("build_ds");
        case PipelineStep::PitchReview:     return QStringLiteral("pitch_review");
    }
    return QStringLiteral("slicer");
}

PipelineStep DsItemManager::stringToStep(const QString &s) {
    if (s == QLatin1String("slicer"))           return PipelineStep::Slicer;
    if (s == QLatin1String("asr"))              return PipelineStep::Asr;
    if (s == QLatin1String("asr_review"))       return PipelineStep::AsrReview;
    if (s == QLatin1String("alignment"))        return PipelineStep::Alignment;
    if (s == QLatin1String("alignment_review")) return PipelineStep::AlignmentReview;
    if (s == QLatin1String("build_csv"))        return PipelineStep::BuildCsv;
    if (s == QLatin1String("midi"))             return PipelineStep::Midi;
    if (s == QLatin1String("build_ds"))         return PipelineStep::BuildDs;
    if (s == QLatin1String("pitch_review"))     return PipelineStep::PitchReview;
    return PipelineStep::Slicer;
}

QString DsItemManager::statusToString(ItemStatus status) {
    switch (status) {
        case ItemStatus::Pending:    return QStringLiteral("pending");
        case ItemStatus::Completed:  return QStringLiteral("completed");
        case ItemStatus::Failed:     return QStringLiteral("failed");
        case ItemStatus::Reviewed:   return QStringLiteral("reviewed");
        case ItemStatus::Unreviewed: return QStringLiteral("unreviewed");
    }
    return QStringLiteral("pending");
}

ItemStatus DsItemManager::stringToStatus(const QString &s) {
    if (s == QLatin1String("completed"))  return ItemStatus::Completed;
    if (s == QLatin1String("failed"))     return ItemStatus::Failed;
    if (s == QLatin1String("reviewed"))   return ItemStatus::Reviewed;
    if (s == QLatin1String("unreviewed")) return ItemStatus::Unreviewed;
    return ItemStatus::Pending;
}

// ── JSON serialization ───────────────────────────────────────────────

nlohmann::json DsItemManager::recordToJson(const DsItemRecord &record) const {
    nlohmann::json j;
    j["version"] = record.version.toStdString();
    j["source_file"] = record.sourceFile.toStdString();
    j["step"] = stepToString(record.step).toStdString();
    j["status"] = statusToString(record.status).toStdString();
    j["timestamp"] = record.timestamp.toString(Qt::ISODate).toStdString();

    if (record.model.has_value()) {
        nlohmann::json mj;
        mj["name"] = record.model->name.toStdString();
        mj["version"] = record.model->version.toStdString();
        mj["path"] = record.model->path.toStdString();
        j["model"] = mj;
    }

    j["params"] = record.params.is_null() ? nlohmann::json::object() : record.params;

    nlohmann::json inArr = nlohmann::json::array();
    for (const auto &s : record.inputs)
        inArr.push_back(s.toStdString());
    j["inputs"] = inArr;

    nlohmann::json outArr = nlohmann::json::array();
    for (const auto &s : record.outputs)
        outArr.push_back(s.toStdString());
    j["outputs"] = outArr;

    return j;
}

DsItemRecord DsItemManager::jsonToRecord(const nlohmann::json &j) const {
    DsItemRecord rec;
    rec.version = JsonHelper::get(j, "version", QString("1.0.0"));
    rec.sourceFile = JsonHelper::get(j, "source_file", QString());
    rec.step = stringToStep(JsonHelper::get(j, "step", QString("slicer")));
    rec.status = stringToStatus(JsonHelper::get(j, "status", QString("pending")));
    rec.timestamp = QDateTime::fromString(
        JsonHelper::get(j, "timestamp", QString()), Qt::ISODate);

    if (j.contains("model") && j["model"].is_object()) {
        ModelInfo mi;
        mi.name = JsonHelper::get(j["model"], "name", QString());
        mi.version = JsonHelper::get(j["model"], "version", QString());
        mi.path = JsonHelper::get(j["model"], "path", QString());
        rec.model = mi;
    }

    rec.params = j.contains("params") && j["params"].is_object()
                     ? j["params"]
                     : nlohmann::json::object();

    if (j.contains("inputs") && j["inputs"].is_array()) {
        for (const auto &v : j["inputs"])
            if (v.is_string())
                rec.inputs.append(QString::fromStdString(v.get<std::string>()));
    }
    if (j.contains("outputs") && j["outputs"].is_array()) {
        for (const auto &v : j["outputs"])
            if (v.is_string())
                rec.outputs.append(QString::fromStdString(v.get<std::string>()));
    }

    return rec;
}

// ── File I/O ─────────────────────────────────────────────────────────

bool DsItemManager::load(const std::filesystem::path &dsitemPath, DsItemRecord &record,
                          std::string &error) const {
    auto j = JsonHelper::loadFile(dsitemPath, error);
    if (!error.empty())
        return false;

    if (!j.is_object()) {
        error = "dsitem file must be a JSON object";
        return false;
    }

    record = jsonToRecord(j);
    return true;
}

bool DsItemManager::save(const std::filesystem::path &dsitemPath, const DsItemRecord &record,
                          std::string &error) const {
    // Ensure parent directory exists
    auto parentDir = dsitemPath.parent_path();
    if (!parentDir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parentDir, ec);
        if (ec) {
            error = "failed to create directory: " + ec.message();
            return false;
        }
    }

    return JsonHelper::saveFile(dsitemPath, recordToJson(record), error);
}

// ── Query ────────────────────────────────────────────────────────────

std::filesystem::path DsItemManager::itemPath(const QString &sourceFile, PipelineStep step) const {
    QFileInfo fi(sourceFile);
    QString baseName = fi.completeBaseName();
    auto stepDir = m_projectRoot / "dstemp" /
                   stepToString(step).toStdString();
    return stepDir / (baseName.toStdString() + ".dsitem");
}

ItemStatus DsItemManager::queryStatus(const QString &sourceFile, PipelineStep step) const {
    auto path = itemPath(sourceFile, step);
    std::string error;
    DsItemRecord record;
    if (!load(path, record, error))
        return ItemStatus::Pending;
    return record.status;
}

// ── Incremental processing ───────────────────────────────────────────

bool DsItemManager::needsProcessing(const QString &sourceFile, PipelineStep step) const {
    auto dsitem = itemPath(sourceFile, step);

    // If .dsitem doesn't exist, needs processing
    if (!std::filesystem::exists(dsitem))
        return true;

    // Load existing record
    std::string error;
    DsItemRecord record;
    if (!load(dsitem, record, error))
        return true;

    // If not completed, needs processing
    if (record.status != ItemStatus::Completed && record.status != ItemStatus::Reviewed)
        return true;

    // Check if source file is newer than the .dsitem timestamp
    auto sourceFsPath = DsDocument::toFsPath(sourceFile);
    std::error_code ec;
    auto sourceTime = std::filesystem::last_write_time(sourceFsPath, ec);
    if (ec)
        return true; // Source file missing or inaccessible

    auto dsitemTime = std::filesystem::last_write_time(dsitem, ec);
    if (ec)
        return true;

    return sourceTime > dsitemTime;
}

// ── Batch query ──────────────────────────────────────────────────────

DsItemManager::StepSummary DsItemManager::summarizeStep(PipelineStep step) const {
    StepSummary summary;

    auto stepDir = m_projectRoot / "dstemp" / stepToString(step).toStdString();
    std::error_code ec;
    if (!std::filesystem::is_directory(stepDir, ec))
        return summary;

    for (const auto &entry : std::filesystem::directory_iterator(stepDir, ec)) {
        if (!entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".dsitem")
            continue;

        ++summary.total;

        std::string error;
        DsItemRecord record;
        if (!load(entry.path(), record, error)) {
            ++summary.pending;
            continue;
        }

        switch (record.status) {
            case ItemStatus::Completed:
            case ItemStatus::Reviewed:
                ++summary.completed;
                break;
            case ItemStatus::Failed:
                ++summary.failed;
                break;
            default:
                ++summary.pending;
                break;
        }
    }

    return summary;
}

// ── Update ───────────────────────────────────────────────────────────

bool DsItemManager::markCompleted(const QString &sourceFile, PipelineStep step,
                                   const QStringList &outputs, std::string &error) {
    auto path = itemPath(sourceFile, step);

    DsItemRecord record;
    std::string loadErr;
    if (std::filesystem::exists(path))
        load(path, record, loadErr); // best-effort load existing

    record.sourceFile = sourceFile;
    record.step = step;
    record.status = ItemStatus::Completed;
    record.outputs = outputs;
    record.timestamp = QDateTime::currentDateTimeUtc();

    return save(path, record, error);
}

bool DsItemManager::markFailed(const QString &sourceFile, PipelineStep step,
                                const QString &errorMsg, std::string &error) {
    auto path = itemPath(sourceFile, step);

    DsItemRecord record;
    std::string loadErr;
    if (std::filesystem::exists(path))
        load(path, record, loadErr); // best-effort load existing

    record.sourceFile = sourceFile;
    record.step = step;
    record.status = ItemStatus::Failed;
    record.timestamp = QDateTime::currentDateTimeUtc();
    record.params["error"] = errorMsg.toStdString();

    return save(path, record, error);
}

} // namespace dstools
