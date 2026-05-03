#include "ProjectSettingsBackend.h"

namespace dstools {

ProjectSettingsBackend::ProjectSettingsBackend(QObject *parent) : ISettingsBackend(parent) {}

void ProjectSettingsBackend::setProject(DsProject *project) {
    m_project = project;
    emit dataChanged();
}

DsProject *ProjectSettingsBackend::project() const {
    return m_project;
}

QJsonObject ProjectSettingsBackend::load() const {
    QJsonObject data;
    if (!m_project)
        return data;

    const auto defaults = m_project->defaults();

    data["globalProvider"] = defaults.globalProvider;
    data["deviceIndex"] = defaults.deviceIndex;

    QJsonObject models;
    for (const auto &[key, cfg] : defaults.taskModels) {
        QJsonObject obj;
        obj["modelPath"] = cfg.modelPath;
        obj["provider"] = cfg.provider;
        obj["forceCpu"] = cfg.forceCpu;
        models[key] = obj;
    }
    data["taskModels"] = models;

    QJsonObject preload;
    for (const auto &[key, cfg] : defaults.preload) {
        QJsonObject obj;
        obj["enabled"] = cfg.enabled;
        obj["count"] = cfg.count;
        preload[key] = obj;
    }
    data["preload"] = preload;

    return data;
}

void ProjectSettingsBackend::save(const QJsonObject &data) {
    if (!m_project)
        return;

    DsProjectDefaults defaults = m_project->defaults();

    defaults.globalProvider = data["globalProvider"].toString("cpu");
    defaults.deviceIndex = data["deviceIndex"].toInt(0);

    const QJsonObject models = data["taskModels"].toObject();
    for (auto it = models.begin(); it != models.end(); ++it) {
        const QJsonObject obj = it.value().toObject();
        TaskModelConfig cfg;
        cfg.modelPath = obj["modelPath"].toString();
        cfg.provider = obj["provider"].toString("cpu");
        cfg.forceCpu = obj["forceCpu"].toBool(false);
        defaults.taskModels[it.key()] = cfg;
    }

    const QJsonObject preload = data["preload"].toObject();
    for (auto it = preload.begin(); it != preload.end(); ++it) {
        const QJsonObject obj = it.value().toObject();
        PreloadConfig cfg;
        cfg.enabled = obj["enabled"].toBool(false);
        cfg.count = obj["count"].toInt(10);
        defaults.preload[it.key()] = cfg;
    }

    m_project->setDefaults(defaults);
}

} // namespace dstools
