#include <dsfw/ConfigTypesJson.h>
#include <dstools/DsProject.h>
#include <dstools/ProjectBackupManager.h>
#include <dsfw/Result.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/Log.h>
#include <dsfw/PathUtils.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include <cmath>
#include <filesystem>
#include <unordered_set>

using dsfw::JsonHelper;

namespace dstools {

using namespace dsfw;

// ── PIMPL ─────────────────────────────────────────────────────────────

struct DsProject::Impl {
    QString m_filePath;
    QString m_workingDirectory;
    std::vector<Item> m_items;
    SlicerState m_slicerState;
    ExportConfig m_exportConfig;
    nlohmann::json m_extraFields; // preserve unknown fields for round-trip
};

// ── Construction / destruction ────────────────────────────────────────

DsProject::DsProject()
    : m_impl(std::make_unique<Impl>()) {}

DsProject::~DsProject() = default;

DsProject::DsProject(DsProject&& other) noexcept
    : m_impl(std::move(other.m_impl)) {}

DsProject& DsProject::operator=(DsProject&& other) noexcept {
    if (this != &other)
        m_impl = std::move(other.m_impl);
    return *this;
}

// ── Slice parsing ─────────────────────────────────────────────────────

static Slice parseSlice(const nlohmann::json& j) {
    Slice s;
    if (j.contains("id") && j["id"].is_string())
        s.id = QString::fromStdString(j["id"].get<std::string>());
    if (j.contains("name") && j["name"].is_string())
        s.name = QString::fromStdString(j["name"].get<std::string>());
    if (j.contains("in") && j["in"].is_number_integer())
        s.inPos = j["in"].get<int64_t>();
    if (j.contains("out") && j["out"].is_number_integer())
        s.outPos = j["out"].get<int64_t>();
    if (j.contains("status") && j["status"].is_string())
        s.status = QString::fromStdString(j["status"].get<std::string>());
    if (j.contains("discardReason") && j["discardReason"].is_string())
        s.discardReason = QString::fromStdString(j["discardReason"].get<std::string>());
    if (j.contains("discardedAt") && j["discardedAt"].is_string())
        s.discardedAt = QString::fromStdString(j["discardedAt"].get<std::string>());

    s.extra = configMapFromJson(j);
    return s;
}

static nlohmann::json serializeSlice(const Slice& s) {
    nlohmann::json j = configMapToJson(s.extra);
    j["id"] = s.id.toStdString();
    if (!s.name.isEmpty())
        j["name"] = s.name.toStdString();
    j["in"] = s.inPos;
    j["out"] = s.outPos;
    if (s.status != QStringLiteral("active"))
        j["status"] = s.status.toStdString();
    if (!s.discardReason.isEmpty())
        j["discardReason"] = s.discardReason.toStdString();
    if (!s.discardedAt.isEmpty())
        j["discardedAt"] = s.discardedAt.toStdString();
    return j;
}

// ── Item parsing ──────────────────────────────────────────────────────

static Item parseItem(const nlohmann::json& j) {
    Item item;
    if (j.contains("id") && j["id"].is_string())
        item.id = QString::fromStdString(j["id"].get<std::string>());
    if (j.contains("name") && j["name"].is_string())
        item.name = QString::fromStdString(j["name"].get<std::string>());
    if (j.contains("speaker") && j["speaker"].is_string())
        item.speaker = QString::fromStdString(j["speaker"].get<std::string>());
    if (j.contains("language") && j["language"].is_string())
        item.language = QString::fromStdString(j["language"].get<std::string>());
    if (j.contains("audioSource") && j["audioSource"].is_string())
        item.audioSource =
            dsfw::PathUtils::toNativeSeparators(QString::fromStdString(j["audioSource"].get<std::string>()));

    if (j.contains("slices") && j["slices"].is_array()) {
        for (const auto& sj : j["slices"])
            item.slices.push_back(parseSlice(sj));
    }

    item.extra = configMapFromJson(j);
    return item;
}

static nlohmann::json serializeItem(const Item& item) {
    nlohmann::json j = configMapToJson(item.extra);
    j["id"] = item.id.toStdString();
    if (!item.name.isEmpty())
        j["name"] = item.name.toStdString();
    if (!item.speaker.isEmpty())
        j["speaker"] = item.speaker.toStdString();
    if (!item.language.isEmpty())
        j["language"] = item.language.toStdString();
    if (!item.audioSource.isEmpty())
        j["audioSource"] = dsfw::PathUtils::toPosixSeparators(item.audioSource).toStdString();

    nlohmann::json slices = nlohmann::json::array();
    for (const auto& s : item.slices)
        slices.push_back(serializeSlice(s));
    j["slices"] = slices;
    return j;
}

// ── File I/O ──────────────────────────────────────────────────────────

Result<DsProject> DsProject::loadFile(const QString& path) {
    DsProject proj;

    if (path.isEmpty()) {
        DSFW_LOG_WARN("io", "DsProject::loadFile: empty path, returning default project");
        return proj;
    }

    auto jsonResult = JsonHelper::loadFile(dsfw::PathUtils::toStdPath(path));
    if (!jsonResult) {
        auto bakPathResult = ProjectBackupManager::findLatestBackup(dsfw::PathUtils::toStdPath(path));
        if (bakPathResult && !bakPathResult.value().empty()) {
            QString bakPath = dsfw::PathUtils::fromStdPath(bakPathResult.value());
            DSFW_LOG_WARN("io", ("DsProject::loadFile: failed, trying backup: " + bakPath.toStdString()).c_str());
            jsonResult = JsonHelper::loadFile(dsfw::PathUtils::toStdPath(bakPath));
            if (jsonResult)
                DSFW_LOG_INFO("io", "DsProject::loadFile: recovered from backup");
        }
    }
    if (!jsonResult) {
        return Result<DsProject>::Error(jsonResult.error());
    }

    const auto& json = jsonResult.value();

    if (!json.is_object()) {
        return Result<DsProject>::Error("Project file must be a JSON object");
    }

    proj.m_impl->m_filePath = path;
    proj.m_impl->m_extraFields = json;

    // Version check
    if (json.contains("version") && json["version"].is_string()) {
        std::string version = json["version"].get<std::string>();
        // Major version must be 3; warn on unknown major versions (backward compat)
        if (!version.empty() && version[0] != '3') {
            DSFW_LOG_WARN(
                "io",
                ("DsProject::load: unsupported project version: " + version + ", attempting compat load").c_str());
        }
    }

    // Config version check (for schema migration, introduced in configVersion=1)
    int configVersion = 0;
    if (json.contains("configVersion") && json["configVersion"].is_number_integer()) {
        configVersion = json["configVersion"].get<int>();
    }
    if (configVersion < 1) {
        DSFW_LOG_INFO("io", "DsProject::load: legacy config format (configVersion < 1), applying compat rules");
    }

    if (json.contains("workingDirectory") && json["workingDirectory"].is_string())
        proj.m_impl->m_workingDirectory =
            dsfw::PathUtils::toNativeSeparators(QString::fromStdString(json["workingDirectory"].get<std::string>()));

    if (json.contains("defaults") && json["defaults"].is_object()) {
        const auto& def = json["defaults"];

        // Legacy hopSize/sampleRate → migrate to exportConfig
        if (def.contains("hopSize") && def["hopSize"].is_number_integer())
            proj.m_impl->m_exportConfig.hopSize = def["hopSize"].get<int>();
        if (def.contains("sampleRate") && def["sampleRate"].is_number_integer())
            proj.m_impl->m_exportConfig.sampleRate = def["sampleRate"].get<int>();

        // v3 slicer config → migrate to slicer.params
        if (def.contains("slicer") && def["slicer"].is_object()) {
            const auto& sl = def["slicer"];
            if (sl.contains("threshold") && sl["threshold"].is_number())
                proj.m_impl->m_slicerState.params.threshold = sl["threshold"].get<double>();
            if (sl.contains("minLength") && sl["minLength"].is_number_integer())
                proj.m_impl->m_slicerState.params.minLength = sl["minLength"].get<int>();
            if (sl.contains("minInterval") && sl["minInterval"].is_number_integer())
                proj.m_impl->m_slicerState.params.minInterval = sl["minInterval"].get<int>();
            if (sl.contains("hopSize") && sl["hopSize"].is_number_integer())
                proj.m_impl->m_slicerState.params.hopSize = sl["hopSize"].get<int>();
            if (sl.contains("maxSilence") && sl["maxSilence"].is_number_integer())
                proj.m_impl->m_slicerState.params.maxSilence = sl["maxSilence"].get<int>();
        }

        // v3 export config → migrate to top-level export
        if (def.contains("export") && def["export"].is_object()) {
            const auto& exp = def["export"];
            if (exp.contains("formats") && exp["formats"].is_array()) {
                for (const auto& f : exp["formats"])
                    if (f.is_string())
                        proj.m_impl->m_exportConfig.formats.append(QString::fromStdString(f.get<std::string>()));
            }
            if (exp.contains("hopSize") && exp["hopSize"].is_number_integer())
                proj.m_impl->m_exportConfig.hopSize = exp["hopSize"].get<int>();
            if (exp.contains("sampleRate") && exp["sampleRate"].is_number_integer())
                proj.m_impl->m_exportConfig.sampleRate = exp["sampleRate"].get<int>();
            if (exp.contains("resampleRate") && exp["resampleRate"].is_number_integer())
                proj.m_impl->m_exportConfig.resampleRate = exp["resampleRate"].get<int>();
            if (exp.contains("includeDiscarded") && exp["includeDiscarded"].is_boolean())
                proj.m_impl->m_exportConfig.includeDiscarded = exp["includeDiscarded"].get<bool>();
        }
    }

    // Parse items
    if (json.contains("items") && json["items"].is_array()) {
        for (const auto& ij : json["items"])
            proj.m_impl->m_items.push_back(parseItem(ij));
    }

    // Parse slicer state
    if (json.contains("slicer") && json["slicer"].is_object()) {
        const auto& sl = json["slicer"];
        if (sl.contains("params") && sl["params"].is_object()) {
            const auto& p = sl["params"];
            if (p.contains("threshold") && p["threshold"].is_number())
                proj.m_impl->m_slicerState.params.threshold = p["threshold"].get<double>();
            if (p.contains("minLength") && p["minLength"].is_number_integer())
                proj.m_impl->m_slicerState.params.minLength = p["minLength"].get<int>();
            if (p.contains("minInterval") && p["minInterval"].is_number_integer())
                proj.m_impl->m_slicerState.params.minInterval = p["minInterval"].get<int>();
            if (p.contains("hopSize") && p["hopSize"].is_number_integer())
                proj.m_impl->m_slicerState.params.hopSize = p["hopSize"].get<int>();
            if (p.contains("maxSilence") && p["maxSilence"].is_number_integer())
                proj.m_impl->m_slicerState.params.maxSilence = p["maxSilence"].get<int>();
        }
        if (sl.contains("audioFiles") && sl["audioFiles"].is_array()) {
            for (const auto& f : sl["audioFiles"])
                if (f.is_string())
                    proj.m_impl->m_slicerState.audioFiles.append(
                        dsfw::PathUtils::toNativeSeparators(QString::fromStdString(f.get<std::string>())));
        }
        if (sl.contains("slicePoints") && sl["slicePoints"].is_object()) {
            for (auto it = sl["slicePoints"].begin(); it != sl["slicePoints"].end(); ++it) {
                if (it->is_array()) {
                    std::vector<double> points;
                    for (const auto& v : *it)
                        if (v.is_number())
                            points.push_back(v.get<double>());
                    proj.m_impl->m_slicerState
                        .slicePoints[dsfw::PathUtils::toNativeSeparators(QString::fromStdString(it.key()))] =
                        std::move(points);
                }
            }
        }
    }

    // Parse top-level export config
    if (json.contains("export") && json["export"].is_object()) {
        const auto& exp = json["export"];
        if (exp.contains("formats") && exp["formats"].is_array()) {
            for (const auto& f : exp["formats"])
                if (f.is_string())
                    proj.m_impl->m_exportConfig.formats.append(QString::fromStdString(f.get<std::string>()));
        }
        if (exp.contains("hopSize") && exp["hopSize"].is_number_integer())
            proj.m_impl->m_exportConfig.hopSize = exp["hopSize"].get<int>();
        if (exp.contains("sampleRate") && exp["sampleRate"].is_number_integer())
            proj.m_impl->m_exportConfig.sampleRate = exp["sampleRate"].get<int>();
        if (exp.contains("resampleRate") && exp["resampleRate"].is_number_integer())
            proj.m_impl->m_exportConfig.resampleRate = exp["resampleRate"].get<int>();
        if (exp.contains("includeDiscarded") && exp["includeDiscarded"].is_boolean())
            proj.m_impl->m_exportConfig.includeDiscarded = exp["includeDiscarded"].get<bool>();
    }

    // Deduplicate export formats (defaults.export + top-level export may overlap)
    proj.m_impl->m_exportConfig.formats.removeDuplicates();

    auto consistencyResult = proj.validateSliceConsistency();
    if (!consistencyResult.ok()) {
        DSFW_LOG_WARN("io", ("DsProject::loadFile: slice consistency issues: " + consistencyResult.error()).c_str());
    }

    auto missingPaths = proj.validateExternalPaths();
    if (!missingPaths.empty()) {
        QStringList missingList;
        for (const auto& p : missingPaths) {
            missingList.append(p);
        }
        DSFW_LOG_WARN("io", ("DsProject::loadFile: " + std::to_string(missingPaths.size()) +
                             " external path(s) not found: " + missingList.join(", ").toStdString())
                                .c_str());
    }

    return proj;
}

Result<void> DsProject::saveFile(const QString& path) const {

    QString targetPath = path.isEmpty() ? m_impl->m_filePath : path;
    if (targetPath.isEmpty()) {
        return Result<void>::Error("No file path specified");
    }

    auto backupResult = ProjectBackupManager::createBackup(dsfw::PathUtils::toStdPath(targetPath));
    if (!backupResult)
        DSFW_LOG_WARN("io", ("DsProject::saveFile: backup failed: " + backupResult.error()).c_str());

    nlohmann::json json = m_impl->m_extraFields.is_object() ? m_impl->m_extraFields : nlohmann::json::object();

    // Remove deprecated 'defaults' section from round-trip data
    json.erase("defaults");

    json["version"] = "3.1.0";
    json["configVersion"] = 1;

    if (!m_impl->m_workingDirectory.isEmpty())
        json["workingDirectory"] = dsfw::PathUtils::toPosixSeparators(m_impl->m_workingDirectory).toStdString();

    // Slicer state (params + audioFiles + slicePoints)
    {
        nlohmann::json slicer = nlohmann::json::object();

        nlohmann::json params = nlohmann::json::object();
        params["threshold"] = m_impl->m_slicerState.params.threshold;
        params["minLength"] = m_impl->m_slicerState.params.minLength;
        params["minInterval"] = m_impl->m_slicerState.params.minInterval;
        params["hopSize"] = m_impl->m_slicerState.params.hopSize;
        params["maxSilence"] = m_impl->m_slicerState.params.maxSilence;
        slicer["params"] = params;

        nlohmann::json audioFiles = nlohmann::json::array();
        for (const auto& f : m_impl->m_slicerState.audioFiles)
            audioFiles.push_back(dsfw::PathUtils::toPosixSeparators(f).toStdString());
        slicer["audioFiles"] = audioFiles;

        nlohmann::json slicePoints = nlohmann::json::object();
        for (const auto& [filePath, points] : m_impl->m_slicerState.slicePoints) {
            if (points.empty())
                continue;
            nlohmann::json arr = nlohmann::json::array();
            for (double t : points)
                arr.push_back(t);
            slicePoints[dsfw::PathUtils::toPosixSeparators(filePath).toStdString()] = arr;
        }
        slicer["slicePoints"] = slicePoints;
        json["slicer"] = slicer;
    }

    // Export config
    {
        nlohmann::json exp = nlohmann::json::object();
        if (!m_impl->m_exportConfig.formats.isEmpty()) {
            nlohmann::json fmtArr = nlohmann::json::array();
            for (const auto& f : m_impl->m_exportConfig.formats)
                fmtArr.push_back(f.toStdString());
            exp["formats"] = fmtArr;
        }
        exp["hopSize"] = m_impl->m_exportConfig.hopSize;
        exp["sampleRate"] = m_impl->m_exportConfig.sampleRate;
        exp["resampleRate"] = m_impl->m_exportConfig.resampleRate;
        exp["includeDiscarded"] = m_impl->m_exportConfig.includeDiscarded;
        json["export"] = exp;
    }

    // Items
    nlohmann::json items = nlohmann::json::array();
    for (const auto& item : m_impl->m_items)
        items.push_back(serializeItem(item));
    json["items"] = items;

    auto saveResult = JsonHelper::saveFile(dsfw::PathUtils::toStdPath(targetPath), json);
    if (!saveResult) {
        return Result<void>::Error(saveResult.error());
    }

    auto pruneResult = ProjectBackupManager::pruneBackups(dsfw::PathUtils::toStdPath(targetPath));
    if (!pruneResult)
        DSFW_LOG_WARN("io", ("DsProject::saveFile: prune failed: " + pruneResult.error()).c_str());

    return Result<void>::Ok();
}

// ── Validation ────────────────────────────────────────────────────────

Result<void> DsProject::validateSliceConsistency() const {
    if (m_impl->m_items.empty())
        return Result<void>::Ok();

    QStringList issues;

    for (const auto& [originalPath, points] : m_impl->m_slicerState.slicePoints) {
        if (points.empty())
            continue;

        QString baseName = QFileInfo(originalPath).completeBaseName();
        int expectedSegments = static_cast<int>(points.size()) + 1;

        int actualSegments = 0;
        for (const auto& item : m_impl->m_items) {
            if (item.id.startsWith(baseName))
                ++actualSegments;
        }

        if (actualSegments == 0) {
            issues.append(QFileInfo(originalPath).fileName() + QStringLiteral(" (未导出)"));
        } else if (actualSegments != expectedSegments) {
            issues.append(
                QFileInfo(originalPath).fileName() +
                QStringLiteral(" (切点数不一致: 期望 %1 段, 实际 %2 段)").arg(expectedSegments).arg(actualSegments));
        } else {
            constexpr double kToleranceUs = 1000.0;
            bool mismatch = false;
            int segIdx = 0;
            for (const auto& item : m_impl->m_items) {
                if (!item.id.startsWith(baseName))
                    continue;
                if (item.slices.empty())
                    continue;
                const auto& slice = item.slices[0];
                double expectedStart = (segIdx == 0) ? 0.0 : points[static_cast<size_t>(segIdx - 1)] * 1e6;
                if (std::abs(expectedStart - static_cast<double>(slice.inPos)) > kToleranceUs) {
                    mismatch = true;
                    break;
                }
                ++segIdx;
            }
            if (mismatch)
                issues.append(QFileInfo(originalPath).fileName() + QStringLiteral(" (切点位置已变化)"));
        }
    }

    for (const auto& audioFile : m_impl->m_slicerState.audioFiles) {
        QString resolved = audioFile;
        if (QDir::isRelativePath(resolved))
            resolved = QDir(m_impl->m_workingDirectory).absoluteFilePath(resolved);
        if (m_impl->m_slicerState.slicePoints.find(resolved) == m_impl->m_slicerState.slicePoints.end()) {
            issues.append(QFileInfo(audioFile).fileName() + QStringLiteral(" (未切片)"));
        }
    }

    if (issues.isEmpty())
        return Result<void>::Ok();

    return Result<void>::Error(issues.join(QStringLiteral("\n")).toStdString());
}

std::vector<QString> DsProject::validateExternalPaths() const {
    std::vector<QString> missing;
    std::unordered_set<QString> checked;

    auto resolvePath = [&](const QString& p) -> QString {
        if (p.isEmpty())
            return {};
        if (QDir::isRelativePath(p) && !m_impl->m_workingDirectory.isEmpty()) {
            return QDir(m_impl->m_workingDirectory).absoluteFilePath(p);
        }
        return p;
    };

    auto checkFile = [&](const QString& p) {
        if (p.isEmpty())
            return;
        auto resolved = resolvePath(p);
        if (checked.count(resolved))
            return;
        checked.insert(resolved);
        if (!QFileInfo::exists(resolved)) {
            missing.push_back(p);
        }
    };

    for (const auto& item : m_impl->m_items) {
        checkFile(item.audioSource);
    }

    for (const auto& audioFile : m_impl->m_slicerState.audioFiles) {
        checkFile(audioFile);
    }

    return missing;
}

Result<void> DsProject::validateSchema() const {
    // Check required top-level fields
    if (m_impl->m_filePath.isEmpty())
        return Result<void>::Error("Project file path is empty");

    // Check items
    for (size_t i = 0; i < m_impl->m_items.size(); ++i) {
        const auto& item = m_impl->m_items[i];
        if (item.id.isEmpty())
            return Result<void>::Error("Item[" + std::to_string(i) + "] has empty id");

        for (size_t j = 0; j < item.slices.size(); ++j) {
            const auto& slice = item.slices[j];
            if (slice.id.isEmpty())
                return Result<void>::Error("Slice[" + std::to_string(j) + "] in item '" +
                                           item.id.toStdString() + "' has empty id");
            if (slice.inPos < 0)
                return Result<void>::Error("Slice[" + std::to_string(j) + "] in item '" +
                                           item.id.toStdString() + "' has negative inPos");
            if (slice.outPos < 0)
                return Result<void>::Error("Slice[" + std::to_string(j) + "] in item '" +
                                           item.id.toStdString() + "' has negative outPos");
            if (slice.inPos > slice.outPos)
                return Result<void>::Error("Slice[" + std::to_string(j) + "] in item '" +
                                           item.id.toStdString() + "' has inPos > outPos");
        }
    }

    // Check slicer params
    const auto& sp = m_impl->m_slicerState.params;
    if (sp.minLength <= 0)
        return Result<void>::Error("Slicer minLength must be positive");
    if (sp.hopSize <= 0)
        return Result<void>::Error("Slicer hopSize must be positive");

    // Check export config
    if (m_impl->m_exportConfig.hopSize <= 0)
        return Result<void>::Error("Export hopSize must be positive");
    if (m_impl->m_exportConfig.sampleRate <= 0)
        return Result<void>::Error("Export sampleRate must be positive");

    return Result<void>::Ok();
}

// ── Properties ────────────────────────────────────────────────────────

QString DsProject::workingDirectory() const {
    return m_impl->m_workingDirectory;
}

void DsProject::setWorkingDirectory(const QString& dir) {
    m_impl->m_workingDirectory = dir;
}

const QString& DsProject::filePath() const {
    return m_impl->m_filePath;
}

const std::vector<Item>& DsProject::items() const {
    return m_impl->m_items;
}

void DsProject::setItems(std::vector<Item> items) {
    m_impl->m_items = std::move(items);
}

const SlicerState& DsProject::slicerState() const {
    return m_impl->m_slicerState;
}

void DsProject::setSlicerState(SlicerState state) {
    m_impl->m_slicerState = std::move(state);
}

const ExportConfig& DsProject::exportConfig() const {
    return m_impl->m_exportConfig;
}

void DsProject::setExportConfig(ExportConfig config) {
    m_impl->m_exportConfig = std::move(config);
}

} // namespace dstools
