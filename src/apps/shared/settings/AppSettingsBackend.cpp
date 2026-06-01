#include "AppSettingsBackend.h"
#include "Keys.h"

#include <QSettings>

namespace dstools {

using namespace settings::inference;

AppSettingsBackend::AppSettingsBackend(QObject *parent) : QObject(parent) {}

QJsonObject AppSettingsBackend::load() const {
    QSettings settings;
    QJsonObject data;

    data["globalProvider"] = settings.value(QLatin1String(kGlobalProvider.path), QStringLiteral("cpu")).toString();
    data["deviceIndex"] = settings.value(QLatin1String(kDeviceIndex.path), 0).toInt();

    QJsonObject models;
    static const char *taskKeys[] = {"asr", "phoneme_alignment", "pitch_extraction", "midi_transcription", "moe_curve"};
    for (const auto &task : taskKeys) {
        settings.beginGroup(QString::fromLatin1(kModelsGroupFmt).arg(QString::fromLatin1(task)));
        QJsonObject cfg;
        cfg["modelPath"] = settings.value(QLatin1String(kModelsModelPath)).toString();
        cfg["provider"] = settings.value(QLatin1String(kModelsProvider), QStringLiteral("cpu")).toString();
        cfg["forceCpu"] = settings.value(QLatin1String(kModelsForceCpu), false).toBool();
        models[QString::fromLatin1(task)] = cfg;
        settings.endGroup();
    }
    data["taskModels"] = models;

    QJsonObject preload;
    static const char *preloadKeys[] = {"phoneme_alignment", "pitch_extraction"};
    for (const auto &task : preloadKeys) {
        settings.beginGroup(QString::fromLatin1(kPreloadGroupFmt).arg(QString::fromLatin1(task)));
        QJsonObject cfg;
        cfg["enabled"] = settings.value(QLatin1String(kPreloadEnabled), false).toBool();
        cfg["count"] = settings.value(QLatin1String(kPreloadCount), 10).toInt();
        preload[QString::fromLatin1(task)] = cfg;
        settings.endGroup();
    }
    data["preload"] = preload;

    QJsonObject g2p;
    g2p["engine"] = settings.value(QLatin1String(kG2pEngine.path), QStringLiteral("pinyin")).toString();
    g2p["dictPath"] = settings.value(QLatin1String(kDictPath.path)).toString();
    data["g2p"] = g2p;

    QJsonObject faConfig;
    faConfig["nonSpeechPh"] = settings.value(QLatin1String(kFaNonSpeechPh.path), QStringLiteral("AP, SP")).toString();
    data["faConfig"] = faConfig;

    QJsonObject pitchConfig;
    pitchConfig["uvVocab"] = settings.value(QLatin1String(kPitchUvVocab.path), QStringLiteral("AP, SP, br, sil")).toString();
    pitchConfig["uvWordCond"] = settings.value(QLatin1String(kPitchUvWordCond.path), 1).toInt();
    pitchConfig["minF0"] = settings.value(QLatin1String(kPitchMinF0.path), 50.0).toDouble();
    pitchConfig["maxF0"] = settings.value(QLatin1String(kPitchMaxF0.path), 1100.0).toDouble();
    data["pitchConfig"] = pitchConfig;

    QJsonObject phNumConfig;
    QJsonObject phNumLangs;
    settings.beginGroup(QLatin1String(kPhNumLanguagesGroup));
    const QStringList langKeys = settings.childGroups();
    for (const QString &lang : langKeys) {
        settings.beginGroup(lang);
        QJsonObject langCfg;
        langCfg["dictPath"] = settings.value(QLatin1String(kPhNumLanguagesDictPath)).toString();
        langCfg["vowelsPath"] = settings.value(QLatin1String(kPhNumLanguagesVowelsPath)).toString();
        langCfg["consonantsPath"] = settings.value(QLatin1String(kPhNumLanguagesConsonantsPath)).toString();
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

    settings.setValue(QLatin1String(kGlobalProvider.path), data["globalProvider"].toString("cpu"));
    settings.setValue(QLatin1String(kDeviceIndex.path), data["deviceIndex"].toInt(0));

    const QJsonObject models = data["taskModels"].toObject();
    for (auto it = models.begin(); it != models.end(); ++it) {
        settings.beginGroup(QString::fromLatin1(kModelsGroupFmt).arg(it.key()));
        const QJsonObject cfg = it.value().toObject();
        settings.setValue(QLatin1String(kModelsModelPath), cfg["modelPath"].toString());
        settings.setValue(QLatin1String(kModelsProvider), cfg["provider"].toString("cpu"));
        settings.setValue(QLatin1String(kModelsForceCpu), cfg["forceCpu"].toBool(false));
        settings.endGroup();
    }

    const QJsonObject preload = data["preload"].toObject();
    for (auto it = preload.begin(); it != preload.end(); ++it) {
        settings.beginGroup(QString::fromLatin1(kPreloadGroupFmt).arg(it.key()));
        const QJsonObject cfg = it.value().toObject();
        settings.setValue(QLatin1String(kPreloadEnabled), cfg["enabled"].toBool(false));
        settings.setValue(QLatin1String(kPreloadCount), cfg["count"].toInt(10));
        settings.endGroup();
    }

    const QJsonObject g2p = data["g2p"].toObject();
    settings.setValue(QLatin1String(kG2pEngine.path), g2p["engine"].toString("pinyin"));
    settings.setValue(QLatin1String(kDictPath.path), g2p["dictPath"].toString());

    const QJsonObject faConfig = data["faConfig"].toObject();
    settings.setValue(QLatin1String(kFaNonSpeechPh.path), faConfig["nonSpeechPh"].toString("AP, SP"));

    const QJsonObject pitchConfig = data["pitchConfig"].toObject();
    settings.setValue(QLatin1String(kPitchUvVocab.path), pitchConfig["uvVocab"].toString("AP, SP, br, sil"));
    settings.setValue(QLatin1String(kPitchUvWordCond.path), pitchConfig["uvWordCond"].toInt(1));
    settings.setValue(QLatin1String(kPitchMinF0.path), pitchConfig["minF0"].toDouble(50.0));
    settings.setValue(QLatin1String(kPitchMaxF0.path), pitchConfig["maxF0"].toDouble(1100.0));

    const QJsonObject phNumConfig = data["phNumConfig"].toObject();
    const QJsonObject phNumLangs = phNumConfig["languages"].toObject();
    settings.beginGroup(QLatin1String(kPhNumLanguagesGroup));
    settings.remove({});
    for (auto it = phNumLangs.begin(); it != phNumLangs.end(); ++it) {
        const QJsonObject langCfg = it.value().toObject();
        settings.beginGroup(it.key());
        settings.setValue(QLatin1String(kPhNumLanguagesDictPath), langCfg["dictPath"].toString());
        settings.setValue(QLatin1String(kPhNumLanguagesVowelsPath), langCfg["vowelsPath"].toString());
        settings.setValue(QLatin1String(kPhNumLanguagesConsonantsPath), langCfg["consonantsPath"].toString());
        settings.endGroup();
    }
    settings.endGroup();
}

} // namespace dstools
