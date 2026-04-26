#pragma once

#include <QJsonObject>
#include <QObject>

namespace dstools {

    class AppSettingsBackend : public QObject {
        Q_OBJECT
    public:
        explicit AppSettingsBackend(QObject *parent = nullptr);

        QJsonObject load() const;
        void save(const QJsonObject &data);

    signals:
        void dataChanged();
    };

} // namespace dstools
