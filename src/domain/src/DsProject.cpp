#include <dstools/DsProject.h>
#include <dsfw/JsonHelper.h>
#include <dstools/PathUtils.h>

#include <filesystem>
#include <QDir>

namespace dstools {

// ── helpers ───────────────────────────────────────────────────────────

static std::filesystem::path toFsPathLocal(const QString &qpath) {
    return dstools::toFsPath(qpath);
}

static std::string qstr(const QString &s) {
    return s.toStdString();
}

static QString fromStd(const std::string &s) {
    return QString::fromStdString(s);
}

// ── Path utilities ────────────────────────────────────────────────────

QString DsProject::toPosixPath(const QString &nativePath) {
    return QDir::fromNativeSeparators(nativePath);
}

QString DsProject::fromPosixPath(const QString &posixPath) {
    return QDir::toNativeSeparators(posixPath);
}

// ── Slice parsing ─────────────────────────────────────────────────────

static Slice parseSlice(const nlohmann::json &j) {
    Slice s;
    if (j.contains("id") && j["id"].is_string())
        s.id = fromStd(j["id"].get<std::string>());
    if (j.contains("name") && j["name"].is_string())
        s.name = fromStd(j["name"].get<std::string>());
    if (j.contains("in") && j["in"].is_number_integer())
        s.inPos = j["in"].get<int64_t>();
    if (j.contains("out") && j["out"].is_number_integer())
        s.outPos = j["out"].get<int64_t>();
    if (j.contains("status") && j["status"].is_string())
        s.status = fromStd(j["status"].get<std::string>());
    if (j.contains("discardReason") && j["discardReason"].is_string())
        s.discardReason = fromStd(j["discardReason"].get<std::string>());
    if (j.contains("discardedAt") && j["discardedAt"].is_string())
        s.discardedAt = fromStd(j["discardedAt"].get<std::string>());

    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.key() != "id" && it.key() != "name" && it.key() != "in" &&
            it.key() != "out" && it.key() != "status" && it.key() != "discardReason" &&
            it.key() != "discardedAt") {
            s.extra[it.key()] = it.value();
        }
    }
    return s;
}

static nlohmann::json serializeSlice(const Slice &s) {
    nlohmann::json j = s.extra.is_object() ? s.extra : nlohmann::json::object();
    j["id"] = qstr(s.id);
    if (!s.name.isEmpty())
        j["name"] = qstr(s.name);
    j["in"] = s.inPos;
    j["out"] = s.outPos;
    if (s.status != QStringLiteral("active"))
        j["status"] = qstr(s.status);
    if (!s.discardReason.isEmpty())
        j["discardReason"] = qstr(s.discardReason);
    if (!s.discardedAt.isEmpty())
        j["discardedAt"] = qstr(s.discardedAt);
    return j;
}

// ── Item parsing ──────────────────────────────────────────────────────

static Item parseItem(const nlohmann::json &j) {
    Item item;
    if (j.contains("id") && j["id"].is_string())
        item.id = fromStd(j["id"].get<std::string>());
    if (j.contains("name") && j["name"].is_string())
        item.name = fromStd(j["name"].get<std::string>());
    if (j.contains("speaker") && j["speaker"].is_string())
        item.speaker = fromStd(j["speaker"].get<std::string>());
    if (j.contains("language") && j["language"].is_string())
        item.language = fromStd(j["language"].get<std::string>());
    if (j.contains("audioSource") && j["audioSource"].is_string())
        item.audioSource = DsProject::fromPosixPath(fromStd(j["audioSource"].get<std::string>()));

    if (j.contains("slices") && j["slices"].is_array()) {
        for (const auto &sj : j["slices"])
            item.slices.push_back(parseSlice(sj));
    }

    for (auto it = j.begin(); it != j.end(); ++it) {
        if (it.key() != "id" && it.key() != "name" && it.key() != "speaker" &&
            it.key() != "language" && it.key() != "audioSource" && it.key() != "slices") {
            item.extra[it.key()] = it.value();
        }
    }
    return item;
}

static nlohmann::json serializeItem(const Item &item) {
    nlohmann::json j = item.extra.is_object() ? item.extra : nlohmann::json::object();
    j["id"] = qstr(item.id);
    if (!item.name.isEmpty())
        j["name"] = qstr(item.name);
    if (!item.speaker.isEmpty())
        j["speaker"] = qstr(item.speaker);
    if (!item.language.isEmpty())
        j["language"] = qstr(item.language);
    if (!item.audioSource.isEmpty())
        j["audioSource"] = qstr(DsProject::toPosixPath(item.audioSource));

    nlohmann::json slices = nlohmann::json::array();
    for (const auto &s : item.slices)
        slices.push_back(serializeSlice(s));
    j["slices"] = slices;
    return j;
}

// ── File I/O ──────────────────────────────────────────────────────────

DsProject DsProject::load(const QString &path, QString &error) {
    DsProject proj;
    error.clear();

    auto jsonResult = JsonHelper::loadFile(toFsPathLocal(path));
    if (!jsonResult) {
        error = fromStd(jsonResult.error());
        return proj;
    }

    const auto &json = jsonResult.value();

    if (!json.is_object()) {
        error = QStringLiteral("Project file must be a JSON object");
        return proj;
    }

    proj.m_filePath = path;
    proj.m_extraFields = json;

    // Version check
    if (json.contains("version") && json["version"].is_string()) {
        std::string version = json["version"].get<std::string>();
        // Major version must be 3; warn on unknown major versions
        if (!version.empty() && version[0] != '3') {
            error = QStringLiteral("不支持的工程文件版本: %1。本程序支持版本 3.x。")
                        .arg(fromStd(version));
            return proj;
        }
    }

    if (json.contains("workingDirectory") && json["workingDirectory"].is_string())
        proj.m_workingDirectory = fromPosixPath(fromStd(json["workingDirectory"].get<std::string>()));

    if (json.contains("defaults") && json["defaults"].is_object()) {
        const auto &def = json["defaults"];

        if (def.contains("models") && def["models"].is_object()) {
            const auto &models = def["models"];

            // Legacy string-format model paths → migrate to taskModels
            static const char *legacyKeys[] = {"asr", "hubert", "game", "rmvpe"};
            for (const char *key : legacyKeys) {
                if (models.contains(key) && models[key].is_string()) {
                    TaskModelConfig cfg;
                    cfg.modelPath = fromStd(models[key].get<std::string>());
                    proj.m_defaults.taskModels[QString::fromLatin1(key)] = std::move(cfg);
                }
            }

            // Parse new object-format task model configs
            for (auto it = models.begin(); it != models.end(); ++it) {
                if (it->is_object()) {
                    const auto &obj = *it;
                    TaskModelConfig cfg;
                    if (obj.contains("processor") && obj["processor"].is_string())
                        cfg.processorId = fromStd(obj["processor"].get<std::string>());
                    if (obj.contains("path") && obj["path"].is_string())
                        cfg.modelPath = fromStd(obj["path"].get<std::string>());
                    if (obj.contains("provider") && obj["provider"].is_string())
                        cfg.provider = fromStd(obj["provider"].get<std::string>());
                    if (obj.contains("deviceId") && obj["deviceId"].is_number_integer())
                        cfg.deviceId = obj["deviceId"].get<int>();
                    if (obj.contains("forceCpu") && obj["forceCpu"].is_boolean())
                        cfg.forceCpu = obj["forceCpu"].get<bool>();

                    for (auto eit = obj.begin(); eit != obj.end(); ++eit) {
                        if (eit.key() != "processor" && eit.key() != "path" &&
                            eit.key() != "provider" && eit.key() != "deviceId" &&
                            eit.key() != "forceCpu") {
                            cfg.extra[eit.key()] = eit.value();
                        }
                    }

                    proj.m_defaults.taskModels[fromStd(it.key())] = std::move(cfg);
                }
            }

            // Legacy gpuIndex → migrate to each task's deviceId
            if (def.contains("gpuIndex") && def["gpuIndex"].is_number_integer()) {
                const int gpuIndex = def["gpuIndex"].get<int>();
                for (auto &[name, cfg] : proj.m_defaults.taskModels) {
                    if (cfg.deviceId == 0 && cfg.provider == "cpu")
                        cfg.deviceId = gpuIndex;
                }
            }
        }

        // Global provider/device
        if (def.contains("globalProvider") && def["globalProvider"].is_string())
            proj.m_defaults.globalProvider = fromStd(def["globalProvider"].get<std::string>());
        if (def.contains("deviceIndex") && def["deviceIndex"].is_number_integer())
            proj.m_defaults.deviceIndex = def["deviceIndex"].get<int>();

        // Legacy hopSize/sampleRate → migrate to exportConfig
        if (def.contains("hopSize") && def["hopSize"].is_number_integer())
            proj.m_exportConfig.hopSize = def["hopSize"].get<int>();
        if (def.contains("sampleRate") && def["sampleRate"].is_number_integer())
            proj.m_exportConfig.sampleRate = def["sampleRate"].get<int>();

        // v3 slicer config → migrate to slicer.params
        if (def.contains("slicer") && def["slicer"].is_object()) {
            const auto &sl = def["slicer"];
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
            const auto &exp = def["export"];
            if (exp.contains("formats") && exp["formats"].is_array()) {
                for (const auto &f : exp["formats"])
                    if (f.is_string())
                        proj.m_exportConfig.formats.append(fromStd(f.get<std::string>()));
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
        for (const auto &ij : json["items"])
            proj.m_items.push_back(parseItem(ij));
    }

    // Parse slicer state
    if (json.contains("slicer") && json["slicer"].is_object()) {
        const auto &sl = json["slicer"];
        if (sl.contains("params") && sl["params"].is_object()) {
            const auto &p = sl["params"];
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
            for (const auto &f : sl["audioFiles"])
                if (f.is_string())
                    proj.m_slicerState.audioFiles.append(fromPosixPath(fromStd(f.get<std::string>())));
        }
        if (sl.contains("slicePoints") && sl["slicePoints"].is_object()) {
            for (auto it = sl["slicePoints"].begin(); it != sl["slicePoints"].end(); ++it) {
                if (it->is_array()) {
                    std::vector<double> points;
                    for (const auto &v : *it)
                        if (v.is_number())
                            points.push_back(v.get<double>());
                    proj.m_slicerState.slicePoints[fromPosixPath(fromStd(it.key()))] = std::move(points);
                }
            }
        }
    }

    // Parse top-level export config
    if (json.contains("export") && json["export"].is_object()) {
        const auto &exp = json["export"];
        if (exp.contains("formats") && exp["formats"].is_array()) {
            for (const auto &f : exp["formats"])
                if (f.is_string())
                    proj.m_exportConfig.formats.append(fromStd(f.get<std::string>()));
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

    return proj;
}

bool DsProject::save(const QString &path, QString &error) const {
    error.clear();

    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty()) {
        error = QStringLiteral("No file path specified");
        return false;
    }

    nlohmann::json json = m_extraFields.is_object() ? m_extraFields : nlohmann::json::object();

    // Remove deprecated 'defaults' section from round-trip data
    json.erase("defaults");

    json["version"] = "3.1.0";

    if (!m_workingDirectory.isEmpty())
        json["workingDirectory"] = qstr(toPosixPath(m_workingDirectory));

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
        for (const auto &f : m_slicerState.audioFiles)
            audioFiles.push_back(qstr(toPosixPath(f)));
        slicer["audioFiles"] = audioFiles;

        nlohmann::json slicePoints = nlohmann::json::object();
        for (const auto &[filePath, points] : m_slicerState.slicePoints) {
            if (points.empty())
                continue;
            nlohmann::json arr = nlohmann::json::array();
            for (double t : points)
                arr.push_back(t);
            slicePoints[qstr(toPosixPath(filePath))] = arr;
        }
        slicer["slicePoints"] = slicePoints;
        json["slicer"] = slicer;
    }

    // Export config
    {
        nlohmann::json exp = nlohmann::json::object();
        if (!m_exportConfig.formats.isEmpty()) {
            nlohmann::json fmtArr = nlohmann::json::array();
            for (const auto &f : m_exportConfig.formats)
                fmtArr.push_back(qstr(f));
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
    for (const auto &item : m_items)
        items.push_back(serializeItem(item));
    json["items"] = items;

    auto saveResult = JsonHelper::saveFile(toFsPathLocal(targetPath), json);
    if (!saveResult) {
        error = fromStd(saveResult.error());
        return false;
    }
    return true;
}

bool DsProject::save(QString &error) const {
    return save(QString(), error);
}

// ── Properties ────────────────────────────────────────────────────────

QString DsProject::workingDirectory() const {
    return m_workingDirectory;
}

void DsProject::setWorkingDirectory(const QString &dir) {
    m_workingDirectory = dir;
}

DsProjectDefaults DsProject::defaults() const {
    return m_defaults;
}

void DsProject::setDefaults(const DsProjectDefaults &defaults) {
    m_defaults = defaults;
}

const QString &DsProject::filePath() const {
    return m_filePath;
}

const std::vector<Item> &DsProject::items() const {
    return m_items;
}

void DsProject::setItems(std::vector<Item> items) {
    m_items = std::move(items);
}

const SlicerState &DsProject::slicerState() const {
    return m_slicerState;
}

void DsProject::setSlicerState(SlicerState state) {
    m_slicerState = std::move(state);
}

const ExportConfig &DsProject::exportConfig() const {
    return m_exportConfig;
}

void DsProject::setExportConfig(ExportConfig config) {
    m_exportConfig = std::move(config);
}

} // namespace dstools
