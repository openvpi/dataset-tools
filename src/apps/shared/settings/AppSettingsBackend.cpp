#include "AppSettingsBackend.h"
#include "AppSettingKeys.h"

#include <QSettings>

namespace dstools {

using namespace AppSettingKeys;

AppSettingsBackend::AppSettingsBackend(QObject *parent) : ISettingsBackend(parent) {}

QJsonObject AppSettingsBackend::load() const {
    QSettings settings;
    QJsonObject data;

    data["globalProvider"] = settings.value(QLatin1String(GlobalProvider), QStringLiteral("cpu")).toString();
    data["deviceIndex"] = settings.value(QLatin1String(DeviceIndex), 0).toInt();

    QJsonObject models;
    static const char *taskKeys[] = {"asr", "phoneme_alignment", "pitch_extraction", "midi_transcription"};
    for (const auto &task : taskKeys) {
        settings.beginGroup(QString::fromLatin1(ModelsGroupFmt).arg(QString::fromLatin1(task)));
        QJsonObject cfg;
        cfg["modelPath"] = settings.value(QLatin1String(ModelsModelPath)).toString();
        cfg["provider"] = settings.value(QLatin1String(ModelsProvider), QStringLiteral("cpu")).toString();
        cfg["forceCpu"] = settings.value(QLatin1String(ModelsForceCpu), false).toBool();
        models[QString::fromLatin1(task)] = cfg;
        settings.endGroup();
    }
    data["taskModels"] = models;

    QJsonObject preload;
    static const char *preloadKeys[] = {"phoneme_alignment", "pitch_extraction"};
    for (const auto &task : preloadKeys) {
        settings.beginGroup(QString::fromLatin1(PreloadGroupFmt).arg(QString::fromLatin1(task)));
        QJsonObject cfg;
        cfg["enabled"] = settings.value(QLatin1String(PreloadEnabled), false).toBool();
        cfg["count"] = settings.value(QLatin1String(PreloadCount), 10).toInt();
        preload[QString::fromLatin1(task)] = cfg;
        settings.endGroup();
    }
    data["preload"] = preload;

    QJsonObject g2p;
    g2p["engine"] = settings.value(QLatin1String(G2pEngine), QStringLiteral("pinyin")).toString();
    g2p["dictPath"] = settings.value(QLatin1String(DictPath)).toString();
    data["g2p"] = g2p;

    QJsonObject faConfig;
    faConfig["nonSpeechPh"] = settings.value(QLatin1String(FaNonSpeechPh), QStringLiteral("AP, SP")).toString();
    data["faConfig"] = faConfig;

    QJsonObject pitchConfig;
    pitchConfig["uvVocab"] = settings.value(QLatin1String(PitchUvVocab), QStringLiteral("AP, SP, br, sil")).toString();
    pitchConfig["uvWordCond"] = settings.value(QLatin1String(PitchUvWordCond), 1).toInt();
    data["pitchConfig"] = pitchConfig;

    QJsonObject phNumConfig;
    QJsonObject phNumLangs;
    settings.beginGroup(QLatin1String(PhNumLanguagesGroup));
    const QStringList langKeys = settings.childGroups();
    for (const QString &lang : langKeys) {
        settings.beginGroup(lang);
        QJsonObject langCfg;
        langCfg["dictPath"] = settings.value(QLatin1String(PhNumLanguagesDictPath)).toString();
        langCfg["vowelsPath"] = settings.value(QLatin1String(PhNumLanguagesVowelsPath)).toString();
        langCfg["consonantsPath"] = settings.value(QLatin1String(PhNumLanguagesConsonantsPath)).toString();
        phNumLangs[lang] = langCfg;
        settings.endGroup();
    }
    settings.endGroup();
    phNumConfig["languages"] = phNumLangs;
    data["phNumConfig"] = phNumConfig;

    return data;
}

void AppSettingsBackend::save(const QJsonObject &data) {
    QSettings settings;

    settings.setValue(QLatin1String(GlobalProvider), data["globalProvider"].toString("cpu"));
    settings.setValue(QLatin1String(DeviceIndex), data["deviceIndex"].toInt(0));

    const QJsonObject models = data["taskModels"].toObject();
    for (auto it = models.begin(); it != models.end(); ++it) {
        settings.beginGroup(QString::fromLatin1(ModelsGroupFmt).arg(it.key()));
        const QJsonObject cfg = it.value().toObject();
        settings.setValue(QLatin1String(ModelsModelPath), cfg["modelPath"].toString());
        settings.setValue(QLatin1String(ModelsProvider), cfg["provider"].toString("cpu"));
        settings.setValue(QLatin1String(ModelsForceCpu), cfg["forceCpu"].toBool(false));
        settings.endGroup();
    }

    const QJsonObject preload = data["preload"].toObject();
    for (auto it = preload.begin(); it != preload.end(); ++it) {
        settings.beginGroup(QString::fromLatin1(PreloadGroupFmt).arg(it.key()));
        const QJsonObject cfg = it.value().toObject();
        settings.setValue(QLatin1String(PreloadEnabled), cfg["enabled"].toBool(false));
        settings.setValue(QLatin1String(PreloadCount), cfg["count"].toInt(10));
        settings.endGroup();
    }

    const QJsonObject g2p = data["g2p"].toObject();
    settings.setValue(QLatin1String(G2pEngine), g2p["engine"].toString("pinyin"));
    settings.setValue(QLatin1String(DictPath), g2p["dictPath"].toString());

    const QJsonObject faConfig = data["faConfig"].toObject();
    settings.setValue(QLatin1String(FaNonSpeechPh), faConfig["nonSpeechPh"].toString("AP, SP"));

    const QJsonObject pitchConfig = data["pitchConfig"].toObject();
    settings.setValue(QLatin1String(PitchUvVocab), pitchConfig["uvVocab"].toString("AP, SP, br, sil"));
    settings.setValue(QLatin1String(PitchUvWordCond), pitchConfig["uvWordCond"].toInt(1));

    const QJsonObject phNumConfig = data["phNumConfig"].toObject();
    const QJsonObject phNumLangs = phNumConfig["languages"].toObject();
    settings.beginGroup(QLatin1String(PhNumLanguagesGroup));
    settings.remove({});
    for (auto it = phNumLangs.begin(); it != phNumLangs.end(); ++it) {
        const QJsonObject langCfg = it.value().toObject();
        settings.beginGroup(it.key());
        settings.setValue(QLatin1String(PhNumLanguagesDictPath), langCfg["dictPath"].toString());
        settings.setValue(QLatin1String(PhNumLanguagesVowelsPath), langCfg["vowelsPath"].toString());
        settings.setValue(QLatin1String(PhNumLanguagesConsonantsPath), langCfg["consonantsPath"].toString());
        settings.endGroup();
    }
    settings.endGroup();
}

} // namespace dstools
