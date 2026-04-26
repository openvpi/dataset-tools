#include <dstools/DsProject.h>
#include <dsfw/ConfigTypesJson.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/Log.h>
#include <dsfw/PathUtils.h>
#include <dstools/Result.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cmath>
#include <filesystem>

namespace dstools {

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

static constexpr int kMaxBackups = 3;

static void rotateBackups(const QString& filePath) {
    if (!QFile::exists(filePath))
        return;

    QFile::remove(filePath + QStringLiteral(".bak"));
    QFile::copy(filePath, filePath + QStringLiteral(".bak"));

    QString lastNewer = filePath + QStringLiteral(".bak.%1").arg(kMaxBackups);
    QFile::remove(lastNewer);

    for (int i = kMaxBackups - 1; i >= 1; --i) {
        QString older = filePath + QStringLiteral(".bak.%1").arg(i);
        QString newer = filePath + QStringLiteral(".bak.%1").arg(i + 1);
        if (QFile::exists(older)) {
            QFile::remove(newer);
            QFile::copy(older, newer);
        }
    }

    QString bak1 = filePath + QStringLiteral(".bak.1");
    QFile::remove(bak1);
    QFile::copy(filePath, bak1);
}

static QString findLatestBackup(const QString& filePath) {
    QString simpleBak = filePath + QStringLiteral(".bak");
    if (QFile::exists(simpleBak))
        return simpleBak;
    for (int i = 1; i <= kMaxBackups; ++i) {
        QString bak = filePath + QStringLiteral(".bak.%1").arg(i);
        if (QFile::exists(bak))
            return bak;
    }
    return {};
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
        QString bakPath = findLatestBackup(path);
        if (!bakPath.isEmpty()) {
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

    proj.m_filePath = path;
    proj.m_extraFields = json;

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

    if (json.contains("workingDirectory") && json["workingDirectory"].is_string())
        proj.m_workingDirectory =
            dsfw::PathUtils::toNativeSeparators(QString::fromStdString(json["workingDirectory"].get<std::string>()));

    if (json.contains("defaults") && json["defaults"].is_object()) {
        const auto& def = json["defaults"];

        // Legacy hopSize/sampleRate → migrate to exportConfig
        if (def.contains("hopSize") && def["hopSize"].is_number_integer())
            proj.m_exportConfig.hopSize = def["hopSize"].get<int>();
        if (def.contains("sampleRate") && def["sampleRate"].is_number_integer())
            proj.m_exportConfig.sampleRate = def["sampleRate"].get<int>();

        // v3 slicer config → migrate to slicer.params
        if (def.contains("slicer") && def["slicer"].is_object()) {
            const auto& sl = def["slicer"];
            if (sl.contains("threshold") && sl["threshold"].is_number())
                proj.m_slicerState.params.threshold = sl["threshold"].get<double>();
            if (sl.contains("minLength") && sl["minLength"].is_number_integer())
                proj.m_slicerState.params.minLength = sl["minLength"].get<int>();
            if (sl.contains("minInterval") && sl["minInterval"].is_number_integer())
                proj.m_slicerState.params.minInterval = sl["minInterval"].get<int>();
            if (sl.contains("hopSize") && sl["hopSize"].is_number_integer())
                proj.m_slicerState.params.hopSize = sl["hopSize"].get<int>();
            if (sl.contains("maxSilence") && sl["maxSilence"].is_number_integer())
                proj.m_slicerState.params.maxSilence = sl["maxSilence"].get<int>();
        }

        // v3 export config → migrate to top-level export
        if (def.contains("export") && def["export"].is_object()) {
            const auto& exp = def["export"];
            if (exp.contains("formats") && exp["formats"].is_array()) {
                for (const auto& f : exp["formats"])
                    if (f.is_string())
                        proj.m_exportConfig.formats.append(QString::fromStdString(f.get<std::string>()));
            }
            if (exp.contains("hopSize") && exp["hopSize"].is_number_integer())
                proj.m_exportConfig.hopSize = exp["hopSize"].get<int>();
            if (exp.contains("sampleRate") && exp["sampleRate"].is_number_integer())
                proj.m_exportConfig.sampleRate = exp["sampleRate"].get<int>();
            if (exp.contains("resampleRate") && exp["resampleRate"].is_number_integer())
                proj.m_exportConfig.resampleRate = exp["resampleRate"].get<int>();
            if (exp.contains("includeDiscarded") && exp["includeDiscarded"].is_boolean())
                proj.m_exportConfig.includeDiscarded = exp["includeDiscarded"].get<bool>();
        }
    }

    // Parse items
    if (json.contains("items") && json["items"].is_array()) {
        for (const auto& ij : json["items"])
            proj.m_items.push_back(parseItem(ij));
    }

    // Parse slicer state
    if (json.contains("slicer") && json["slicer"].is_object()) {
        const auto& sl = json["slicer"];
        if (sl.contains("params") && sl["params"].is_object()) {
            const auto& p = sl["params"];
            if (p.contains("threshold") && p["threshold"].is_number())
                proj.m_slicerState.params.threshold = p["threshold"].get<double>();
            if (p.contains("minLength") && p["minLength"].is_number_integer())
                proj.m_slicerState.params.minLength = p["minLength"].get<int>();
            if (p.contains("minInterval") && p["minInterval"].is_number_integer())
                proj.m_slicerState.params.minInterval = p["minInterval"].get<int>();
            if (p.contains("hopSize") && p["hopSize"].is_number_integer())
                proj.m_slicerState.params.hopSize = p["hopSize"].get<int>();
            if (p.contains("maxSilence") && p["maxSilence"].is_number_integer())
                proj.m_slicerState.params.maxSilence = p["maxSilence"].get<int>();
        }
        if (sl.contains("audioFiles") && sl["audioFiles"].is_array()) {
            for (const auto& f : sl["audioFiles"])
                if (f.is_string())
                    proj.m_slicerState.audioFiles.append(
                        dsfw::PathUtils::toNativeSeparators(QString::fromStdString(f.get<std::string>())));
        }
        if (sl.contains("slicePoints") && sl["slicePoints"].is_object()) {
            for (auto it = sl["slicePoints"].begin(); it != sl["slicePoints"].end(); ++it) {
                if (it->is_array()) {
                    std::vector<double> points;
                    for (const auto& v : *it)
                        if (v.is_number())
                            points.push_back(v.get<double>());
                    proj.m_slicerState
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
                    proj.m_exportConfig.formats.append(QString::fromStdString(f.get<std::string>()));
        }
        if (exp.contains("hopSize") && exp["hopSize"].is_number_integer())
            proj.m_exportConfig.hopSize = exp["hopSize"].get<int>();
        if (exp.contains("sampleRate") && exp["sampleRate"].is_number_integer())
            proj.m_exportConfig.sampleRate = exp["sampleRate"].get<int>();
        if (exp.contains("resampleRate") && exp["resampleRate"].is_number_integer())
            proj.m_exportConfig.resampleRate = exp["resampleRate"].get<int>();
        if (exp.contains("includeDiscarded") && exp["includeDiscarded"].is_boolean())
            proj.m_exportConfig.includeDiscarded = exp["includeDiscarded"].get<bool>();
    }

    // Deduplicate export formats (defaults.export + top-level export may overlap)
    proj.m_exportConfig.formats.removeDuplicates();

    auto consistencyResult = proj.validateSliceConsistency();
    if (!consistencyResult.ok()) {
        DSFW_LOG_WARN("io",
                      ("DsProject::loadFile: slice consistency issues: " + consistencyResult.error()).c_str());
    }

    return proj;
}

Result<void> DsProject::saveFile(const QString& path) const {

    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty()) {
        return Result<void>::Error("No file path specified");
    }

    rotateBackups(targetPath);

    nlohmann::json json = m_extraFields.is_object() ? m_extraFields : nlohmann::json::object();

    // Remove deprecated 'defaults' section from round-trip data
    json.erase("defaults");

    json["version"] = "3.1.0";

    if (!m_workingDirectory.isEmpty())
        json["workingDirectory"] = dsfw::PathUtils::toPosixSeparators(m_workingDirectory).toStdString();

    // Slicer state (params + audioFiles + slicePoints)
    {
        nlohmann::json slicer = nlohmann::json::object();

        nlohmann::json params = nlohmann::json::object();
        params["threshold"] = m_slicerState.params.threshold;
        params["minLength"] = m_slicerState.params.minLength;
        params["minInterval"] = m_slicerState.params.minInterval;
        params["hopSize"] = m_slicerState.params.hopSize;
        params["maxSilence"] = m_slicerState.params.maxSilence;
        slicer["params"] = params;

        nlohmann::json audioFiles = nlohmann::json::array();
        for (const auto& f : m_slicerState.audioFiles)
            audioFiles.push_back(dsfw::PathUtils::toPosixSeparators(f).toStdString());
        slicer["audioFiles"] = audioFiles;

        nlohmann::json slicePoints = nlohmann::json::object();
        for (const auto& [filePath, points] : m_slicerState.slicePoints) {
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
        if (!m_exportConfig.formats.isEmpty()) {
            nlohmann::json fmtArr = nlohmann::json::array();
            for (const auto& f : m_exportConfig.formats)
                fmtArr.push_back(f.toStdString());
            exp["formats"] = fmtArr;
        }
        exp["hopSize"] = m_exportConfig.hopSize;
        exp["sampleRate"] = m_exportConfig.sampleRate;
        exp["resampleRate"] = m_exportConfig.resampleRate;
        exp["includeDiscarded"] = m_exportConfig.includeDiscarded;
        json["export"] = exp;
    }

    // Items
    nlohmann::json items = nlohmann::json::array();
    for (const auto& item : m_items)
        items.push_back(serializeItem(item));
    json["items"] = items;

    auto saveResult = JsonHelper::saveFile(dsfw::PathUtils::toStdPath(targetPath), json);
    if (!saveResult) {
        return Result<void>::Error(saveResult.error());
    }
    return Result<void>::Ok();
}

// ── Validation ────────────────────────────────────────────────────────

Result<void> DsProject::validateSliceConsistency() const {
    if (m_items.empty())
        return Result<void>::Ok();

    QStringList issues;

    for (const auto &[originalPath, points] : m_slicerState.slicePoints) {
        if (points.empty())
            continue;

        QString baseName = QFileInfo(originalPath).completeBaseName();
        int expectedSegments = static_cast<int>(points.size()) + 1;

        int actualSegments = 0;
        for (const auto &item : m_items) {
            if (item.id.startsWith(baseName))
                ++actualSegments;
        }

        if (actualSegments == 0) {
            issues.append(QFileInfo(originalPath).fileName() + QStringLiteral(" (未导出)"));
        } else if (actualSegments != expectedSegments) {
            issues.append(QFileInfo(originalPath).fileName() +
                          QStringLiteral(" (切点数不一致: 期望 %1 段, 实际 %2 段)")
                              .arg(expectedSegments)
                              .arg(actualSegments));
        } else {
            constexpr double kToleranceUs = 1000.0;
            bool mismatch = false;
            int segIdx = 0;
            for (const auto &item : m_items) {
                if (!item.id.startsWith(baseName))
                    continue;
                if (item.slices.empty())
                    continue;
                const auto &slice = item.slices[0];
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

    for (const auto &audioFile : m_slicerState.audioFiles) {
        QString resolved = audioFile;
        if (QDir::isRelativePath(resolved))
            resolved = QDir(m_workingDirectory).absoluteFilePath(resolved);
        if (m_slicerState.slicePoints.find(resolved) == m_slicerState.slicePoints.end()) {
            issues.append(QFileInfo(audioFile).fileName() + QStringLiteral(" (未切片)"));
        }
    }

    if (issues.isEmpty())
        return Result<void>::Ok();

    return Result<void>::Error(issues.join(QStringLiteral("\n")).toStdString());
}

// ── Properties ────────────────────────────────────────────────────────

QString DsProject::workingDirectory() const {
    return m_workingDirectory;
}

void DsProject::setWorkingDirectory(const QString& dir) {
    m_workingDirectory = dir;
}

const QString& DsProject::filePath() const {
    return m_filePath;
}

const std::vector<Item>& DsProject::items() const {
    return m_items;
}

void DsProject::setItems(std::vector<Item> items) {
    m_items = std::move(items);
}

const SlicerState& DsProject::slicerState() const {
    return m_slicerState;
}

void DsProject::setSlicerState(SlicerState state) {
    m_slicerState = std::move(state);
}

const ExportConfig& DsProject::exportConfig() const {
    return m_exportConfig;
}

void DsProject::setExportConfig(ExportConfig config) {
    m_exportConfig = std::move(config);
}

} // namespace dstools
