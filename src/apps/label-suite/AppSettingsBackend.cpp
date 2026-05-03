#include "AppSettingsBackend.h"

#include <QSettings>

namespace dstools {

AppSettingsBackend::AppSettingsBackend(QObject *parent) : ISettingsBackend(parent) {}

QJsonObject AppSettingsBackend::load() const {
    QSettings settings;
    QJsonObject data;

    data["globalProvider"] = settings.value(QStringLiteral("Settings/globalProvider"), QStringLiteral("cpu")).toString();
    data["deviceIndex"] = settings.value(QStringLiteral("Settings/deviceIndex"), 0).toInt();

    QJsonObject models;
    static const char *taskKeys[] = {"asr", "phoneme_alignment", "pitch_extraction", "midi_transcription"};
    for (const auto &task : taskKeys) {
        settings.beginGroup(QStringLiteral("Settings/Models/%1").arg(QString::fromLatin1(task)));
        QJsonObject cfg;
        cfg["modelPath"] = settings.value(QStringLiteral("modelPath")).toString();
        cfg["provider"] = settings.value(QStringLiteral("provider"), QStringLiteral("cpu")).toString();
        cfg["forceCpu"] = settings.value(QStringLiteral("forceCpu"), false).toBool();
        models[QString::fromLatin1(task)] = cfg;
        settings.endGroup();
    }
    data["taskModels"] = models;

    QJsonObject preload;
    static const char *preloadKeys[] = {"phoneme_alignment", "pitch_extraction"};
    for (const auto &task : preloadKeys) {
        settings.beginGroup(QStringLiteral("Settings/Preload/%1").arg(QString::fromLatin1(task)));
        QJsonObject cfg;
        cfg["enabled"] = settings.value(QStringLiteral("enabled"), false).toBool();
        cfg["count"] = settings.value(QStringLiteral("count"), 10).toInt();
        preload[QString::fromLatin1(task)] = cfg;
        settings.endGroup();
    }
    data["preload"] = preload;

    QJsonObject g2p;
    g2p["engine"] = settings.value(QStringLiteral("Settings/g2pEngine"), QStringLiteral("pinyin")).toString();
    g2p["dictPath"] = settings.value(QStringLiteral("Settings/dictPath")).toString();
    data["g2p"] = g2p;

    return data;
}

void AppSettingsBackend::save(const QJsonObject &data) {
    QSettings settings;

    settings.setValue(QStringLiteral("Settings/globalProvider"), data["globalProvider"].toString("cpu"));
    settings.setValue(QStringLiteral("Settings/deviceIndex"), data["deviceIndex"].toInt(0));

    const QJsonObject models = data["taskModels"].toObject();
    for (auto it = models.begin(); it != models.end(); ++it) {
        settings.beginGroup(QStringLiteral("Settings/Models/%1").arg(it.key()));
        const QJsonObject cfg = it.value().toObject();
        settings.setValue(QStringLiteral("modelPath"), cfg["modelPath"].toString());
        settings.setValue(QStringLiteral("provider"), cfg["provider"].toString("cpu"));
        settings.setValue(QStringLiteral("forceCpu"), cfg["forceCpu"].toBool(false));
        settings.endGroup();
    }

    const QJsonObject preload = data["preload"].toObject();
    for (auto it = preload.begin(); it != preload.end(); ++it) {
        settings.beginGroup(QStringLiteral("Settings/Preload/%1").arg(it.key()));
        const QJsonObject cfg = it.value().toObject();
        settings.setValue(QStringLiteral("enabled"), cfg["enabled"].toBool(false));
        settings.setValue(QStringLiteral("count"), cfg["count"].toInt(10));
        settings.endGroup();
    }

    const QJsonObject g2p = data["g2p"].toObject();
    settings.setValue(QStringLiteral("Settings/g2pEngine"), g2p["engine"].toString("pinyin"));
    settings.setValue(QStringLiteral("Settings/dictPath"), g2p["dictPath"].toString());
}

} // namespace dstools
