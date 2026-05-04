#pragma once

#include <QString>
#include <QJsonObject>

#include <dstools/DsProject.h>

#include "ISettingsBackend.h"

namespace dstools {

inline TaskModelConfig readModelConfig(ISettingsBackend *settings, const QString &taskKey) {
    TaskModelConfig cfg;

    if (!settings)
        return cfg;

    const QJsonObject data = settings->load();

    const QJsonObject models = data[QStringLiteral("taskModels")].toObject();
    const QJsonObject model = models[taskKey].toObject();

    cfg.modelPath = model[QStringLiteral("modelPath")].toString();
    cfg.provider = model[QStringLiteral("provider")].toString();

    if (cfg.provider.isEmpty())
        cfg.provider = data[QStringLiteral("globalProvider")].toString(QStringLiteral("cpu"));

    cfg.deviceId = data[QStringLiteral("deviceIndex")].toInt(0);

    return cfg;
}

} // namespace dstools
