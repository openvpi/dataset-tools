#pragma once

#include <QJsonObject>
#include <QObject>

namespace dstools {

class ISettingsBackend : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~ISettingsBackend() override = default;

    virtual QJsonObject load() const = 0;
    virtual void save(const QJsonObject &data) = 0;

signals:
    void dataChanged();
};

} // namespace dstools
