#include <dstools/DsProject.h>
#include <dsfw/JsonHelper.h>
#include <dstools/PathUtils.h>

#include <filesystem>

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
    proj.m_extraFields = json;  // store everything for round-trip

    // Extract known fields
    if (json.contains("workingDirectory") && json["workingDirectory"].is_string())
        proj.m_workingDirectory = fromStd(json["workingDirectory"].get<std::string>());

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

                    // Store remaining keys in extra
                    for (auto eit = obj.begin(); eit != obj.end(); ++eit) {
                        if (eit.key() != "processor" && eit.key() != "path" &&
                            eit.key() != "provider" && eit.key() != "deviceId") {
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
        if (def.contains("hopSize") && def["hopSize"].is_number_integer())
            proj.m_defaults.hopSize = def["hopSize"].get<int>();
        if (def.contains("sampleRate") && def["sampleRate"].is_number_integer())
            proj.m_defaults.sampleRate = def["sampleRate"].get<int>();
    }

    return proj;
}

bool DsProject::save(const QString &path, QString &error) const {
    error.clear();

    QString targetPath = path.isEmpty() ? m_filePath : path;
    if (targetPath.isEmpty()) {
        error = QStringLiteral("No file path specified");
        return false;
    }

    // Start from extra fields to preserve unknown keys
    nlohmann::json json = m_extraFields.is_object() ? m_extraFields : nlohmann::json::object();

    json["version"] = "1.0.0";

    if (!m_workingDirectory.isEmpty())
        json["workingDirectory"] = qstr(m_workingDirectory);

    // Build defaults object, merging with any existing extra fields
    nlohmann::json def = json.contains("defaults") && json["defaults"].is_object()
                             ? json["defaults"]
                             : nlohmann::json::object();

    nlohmann::json models = nlohmann::json::object();

    // Write new object-format task model configs
    for (const auto &[taskName, cfg] : m_defaults.taskModels) {
        nlohmann::json obj = cfg.extra.is_object() ? cfg.extra : nlohmann::json::object();
        if (!cfg.processorId.isEmpty())
            obj["processor"] = qstr(cfg.processorId);
        if (!cfg.modelPath.isEmpty())
            obj["path"] = qstr(cfg.modelPath);
        obj["provider"] = qstr(cfg.provider);
        obj["deviceId"] = cfg.deviceId;
        models[qstr(taskName)] = obj;
    }

    def["models"] = models;
    def["hopSize"] = m_defaults.hopSize;
    def["sampleRate"] = m_defaults.sampleRate;

    json["defaults"] = def;

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

} // namespace dstools
