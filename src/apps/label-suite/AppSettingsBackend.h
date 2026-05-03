#pragma once

#include <ISettingsBackend.h>

namespace dstools {

class AppSettingsBackend : public ISettingsBackend {
    Q_OBJECT
public:
    explicit AppSettingsBackend(QObject *parent = nullptr);

    QJsonObject load() const override;
    void save(const QJsonObject &data) override;
};

} // namespace dstools
