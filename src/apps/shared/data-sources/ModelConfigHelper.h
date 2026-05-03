#pragma once

#include <QString>

namespace dstools {

class ISettingsBackend;

struct TaskModelConfig {
    QString modelPath;
    QString provider;
    int deviceId = 0;
};

TaskModelConfig readModelConfig(ISettingsBackend *settings, const QString &taskKey);

} // namespace dstools
