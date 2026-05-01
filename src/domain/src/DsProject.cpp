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

    std::string err;
    auto json = JsonHelper::loadFile(toFsPathLocal(path), err);
    if (!err.empty()) {
        error = fromStd(err);
        return proj;
    }

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
            if (models.contains("asr") && models["asr"].is_string())
                proj.m_defaults.asrModelPath = fromStd(models["asr"].get<std::string>());
            if (models.contains("hubert") && models["hubert"].is_string())
                proj.m_defaults.hubertModelPath = fromStd(models["hubert"].get<std::string>());
            if (models.contains("game") && models["game"].is_string())
                proj.m_defaults.gameModelPath = fromStd(models["game"].get<std::string>());
            if (models.contains("rmvpe") && models["rmvpe"].is_string())
                proj.m_defaults.rmvpeModelPath = fromStd(models["rmvpe"].get<std::string>());
        }

        if (def.contains("gpuIndex") && def["gpuIndex"].is_number_integer())
            proj.m_defaults.gpuIndex = def["gpuIndex"].get<int>();
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

    nlohmann::json models = def.contains("models") && def["models"].is_object()
                                ? def["models"]
                                : nlohmann::json::object();

    if (!m_defaults.asrModelPath.isEmpty())
        models["asr"] = qstr(m_defaults.asrModelPath);
    if (!m_defaults.hubertModelPath.isEmpty())
        models["hubert"] = qstr(m_defaults.hubertModelPath);
    if (!m_defaults.gameModelPath.isEmpty())
        models["game"] = qstr(m_defaults.gameModelPath);
    if (!m_defaults.rmvpeModelPath.isEmpty())
        models["rmvpe"] = qstr(m_defaults.rmvpeModelPath);

    def["models"] = models;
    def["gpuIndex"] = m_defaults.gpuIndex;
    def["hopSize"] = m_defaults.hopSize;
    def["sampleRate"] = m_defaults.sampleRate;

    json["defaults"] = def;

    std::string err;
    if (!JsonHelper::saveFile(toFsPathLocal(targetPath), json, err)) {
        error = fromStd(err);
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
