#include "SettingsSerializer.h"

#include <QCheckBox>
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>

static QString readString(const QJsonObject &obj, const QString &key, const QString &defaultValue = {}) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->isString())
        return defaultValue;
    return it->toString();
}

static int readInt(const QJsonObject &obj, const QString &key, int defaultValue = 0) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->isDouble())
        return defaultValue;
    return it->toInt(defaultValue);
}

static bool readBool(const QJsonObject &obj, const QString &key, bool defaultValue = false) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->isBool())
        return defaultValue;
    return it->toBool(defaultValue);
}

namespace dstools {

    QString SettingsSerializer::effectiveProvider(QComboBox *providerCombo, QCheckBox *forceCpu) {
        if (forceCpu && forceCpu->isChecked())
            return QStringLiteral("cpu");
        return providerCombo->currentText().split(' ').first();
    }

    QJsonObject SettingsSerializer::modelToJson(QLineEdit *pathEdit, QCheckBox *forceCpu, QComboBox *providerCombo) {
        QJsonObject cfg;
        cfg["modelPath"] = pathEdit->text();
        cfg["provider"] = effectiveProvider(providerCombo, forceCpu);
        cfg["forceCpu"] = forceCpu->isChecked();
        return cfg;
    }

    void SettingsSerializer::modelFromJson(const QJsonObject &models, const QString &key, QLineEdit *pathEdit,
                                           QCheckBox *forceCpu) {
        auto it = models.find(key);
        if (it != models.end()) {
            auto obj = it.value().toObject();
            pathEdit->setText(readString(obj, QStringLiteral("modelPath")));
            forceCpu->setChecked(readBool(obj, QStringLiteral("forceCpu")));
        }
    }

    QJsonObject SettingsSerializer::preloadToJson(QCheckBox *enabledBox, QSpinBox *countSpin) {
        QJsonObject cfg;
        cfg["enabled"] = enabledBox->isChecked();
        cfg["count"] = countSpin->value();
        return cfg;
    }

    void SettingsSerializer::preloadFromJson(const QJsonObject &preload, const QString &key, QCheckBox *enabledBox,
                                             QSpinBox *countSpin, int defaultCount) {
        auto it = preload.find(key);
        if (it != preload.end()) {
            auto obj = it.value().toObject();
            enabledBox->setChecked(readBool(obj, QStringLiteral("enabled")));
            countSpin->setValue(readInt(obj, QStringLiteral("count"), defaultCount));
        }
    }

    QJsonObject SettingsSerializer::toJson(QComboBox *providerCombo, QComboBox *deviceCombo, QLineEdit *asrModelPath,
                                           QCheckBox *asrForceCpu, QLineEdit *faModelPath, QCheckBox *faForceCpu,
                                           QLineEdit *pitchModelPath, QCheckBox *pitchForceCpu,
                                           QLineEdit *midiModelPath, QCheckBox *midiForceCpu, QLineEdit *moeModelPath,
                                           QCheckBox *moeForceCpu, QCheckBox *faPreloadEnabled,
                                           QSpinBox *faPreloadCount, QCheckBox *pitchPreloadEnabled,
                                           QSpinBox *pitchPreloadCount, QComboBox *g2pEngineCombo, QLineEdit *dictPath,
                                           QLineEdit *faNonSpeechPh, QLineEdit *uvVocab, QComboBox *uvWordCondCombo,
                                           QDoubleSpinBox *f0MinSpin, QDoubleSpinBox *f0MaxSpin,
                                           QTableWidget *phNumTable, QLineEdit *phNumSpecialWords,
                                           const std::function<QLineEdit *(QWidget *)> &searchLineEditInPathCell) {
        QJsonObject settingsData;

        settingsData["globalProvider"] = providerCombo->currentText();
        settingsData["deviceIndex"] = deviceCombo->currentData().toInt();

        QJsonObject models;
        models["asr"] = modelToJson(asrModelPath, asrForceCpu, providerCombo);
        models["phoneme_alignment"] = modelToJson(faModelPath, faForceCpu, providerCombo);
        models["pitch_extraction"] = modelToJson(pitchModelPath, pitchForceCpu, providerCombo);
        models["midi_transcription"] = modelToJson(midiModelPath, midiForceCpu, providerCombo);
        models["moe_curve"] = modelToJson(moeModelPath, moeForceCpu, providerCombo);
        settingsData["taskModels"] = models;

        QJsonObject preload;
        preload["phoneme_alignment"] = preloadToJson(faPreloadEnabled, faPreloadCount);
        preload["pitch_extraction"] = preloadToJson(pitchPreloadEnabled, pitchPreloadCount);
        settingsData["preload"] = preload;

        QJsonObject g2p;
        g2p["engine"] = g2pEngineCombo->currentData().toString();
        g2p["dictPath"] = dictPath->text();
        settingsData["g2p"] = g2p;

        QJsonObject faConfig;
        faConfig["nonSpeechPh"] = faNonSpeechPh->text().trimmed().isEmpty() ? faNonSpeechPh->placeholderText()
                                                                            : faNonSpeechPh->text().trimmed();
        settingsData["faConfig"] = faConfig;

        QJsonObject pitchConfig;
        pitchConfig["uvVocab"] =
            uvVocab->text().trimmed().isEmpty() ? uvVocab->placeholderText() : uvVocab->text().trimmed();
        pitchConfig["uvWordCond"] = uvWordCondCombo->currentData().toInt();
        pitchConfig["minF0"] = f0MinSpin->value();
        pitchConfig["maxF0"] = f0MaxSpin->value();
        settingsData["pitchConfig"] = pitchConfig;

        QJsonObject phNumConfig;
        phNumConfig["specialWords"] = phNumSpecialWords->text().trimmed();
        QJsonObject phNumLangs;
        for (int r = 0; r < phNumTable->rowCount(); ++r) {
            auto *langItem = phNumTable->item(r, 0);
            if (!langItem || langItem->text().trimmed().isEmpty())
                continue;
            QString lang = langItem->text().trimmed();
            QJsonObject langCfg;
            auto *cellWidget1 = phNumTable->cellWidget(r, 1);
            if (auto *le = searchLineEditInPathCell(cellWidget1))
                langCfg["dictPath"] = le->text().trimmed();
            auto *cellWidget2 = phNumTable->cellWidget(r, 2);
            if (auto *le = searchLineEditInPathCell(cellWidget2))
                langCfg["vowelsPath"] = le->text().trimmed();
            auto *cellWidget3 = phNumTable->cellWidget(r, 3);
            if (auto *le = searchLineEditInPathCell(cellWidget3))
                langCfg["consonantsPath"] = le->text().trimmed();
            phNumLangs[lang] = langCfg;
        }
        phNumConfig["languages"] = phNumLangs;
        settingsData["phNumConfig"] = phNumConfig;

        return settingsData;
    }

    static double readDouble(const QJsonObject &obj, const QString &key, double defaultValue = 0.0) {
        auto it = obj.find(key);
        if (it == obj.end() || !it->isDouble())
            return defaultValue;
        return it->toDouble(defaultValue);
    }

    void SettingsSerializer::fromJson(const QJsonObject &settingsData, QComboBox *providerCombo, QComboBox *deviceCombo,
                                      QLineEdit *asrModelPath, QCheckBox *asrForceCpu, QLineEdit *faModelPath,
                                      QCheckBox *faForceCpu, QLineEdit *pitchModelPath, QCheckBox *pitchForceCpu,
                                      QLineEdit *midiModelPath, QCheckBox *midiForceCpu, QLineEdit *moeModelPath,
                                      QCheckBox *moeForceCpu, QCheckBox *faPreloadEnabled, QSpinBox *faPreloadCount,
                                      QCheckBox *pitchPreloadEnabled, QSpinBox *pitchPreloadCount,
                                      QComboBox *g2pEngineCombo, QLineEdit *dictPath, QLineEdit *faNonSpeechPh,
                                      QLineEdit *uvVocab, QComboBox *uvWordCondCombo, QDoubleSpinBox *f0MinSpin,
                                      QDoubleSpinBox *f0MaxSpin, QTableWidget *phNumTable, QLineEdit *phNumSpecialWords,
                                      const std::function<QWidget *()> &createPhNumPathCell,
                                      const std::function<QLineEdit *(QWidget *)> &searchLineEditInPathCell) {
        {
            QString provider = readString(settingsData, QStringLiteral("globalProvider"), QStringLiteral("cpu"));
            int idx = providerCombo->findText(provider);
            if (idx >= 0)
                providerCombo->setCurrentIndex(idx);

            int deviceIndex = readInt(settingsData, QStringLiteral("deviceIndex"), 0);
            for (int i = 0; i < deviceCombo->count(); ++i) {
                if (deviceCombo->itemData(i).toInt() == deviceIndex) {
                    deviceCombo->setCurrentIndex(i);
                    break;
                }
            }
        }

        const QJsonObject models = settingsData["taskModels"].toObject();
        modelFromJson(models, QStringLiteral("asr"), asrModelPath, asrForceCpu);
        modelFromJson(models, QStringLiteral("phoneme_alignment"), faModelPath, faForceCpu);
        modelFromJson(models, QStringLiteral("pitch_extraction"), pitchModelPath, pitchForceCpu);
        modelFromJson(models, QStringLiteral("midi_transcription"), midiModelPath, midiForceCpu);
        modelFromJson(models, QStringLiteral("moe_curve"), moeModelPath, moeForceCpu);

        const QJsonObject preload = settingsData["preload"].toObject();
        preloadFromJson(preload, QStringLiteral("phoneme_alignment"), faPreloadEnabled, faPreloadCount, 10);
        preloadFromJson(preload, QStringLiteral("pitch_extraction"), pitchPreloadEnabled, pitchPreloadCount, 10);

        const QJsonObject g2p = settingsData["g2p"].toObject();
        {
            QString engine = readString(g2p, QStringLiteral("engine"), QStringLiteral("pinyin"));
            for (int i = 0; i < g2pEngineCombo->count(); ++i) {
                if (g2pEngineCombo->itemData(i).toString() == engine) {
                    g2pEngineCombo->setCurrentIndex(i);
                    break;
                }
            }
            dictPath->setText(readString(g2p, QStringLiteral("dictPath")));
            dictPath->setEnabled(engine == QStringLiteral("dictionary"));
        }

        const QJsonObject faConfig = settingsData["faConfig"].toObject();
        { faNonSpeechPh->setText(readString(faConfig, QStringLiteral("nonSpeechPh"), QStringLiteral("AP, SP"))); }

        const QJsonObject pitchConfig = settingsData["pitchConfig"].toObject();
        {
            uvVocab->setText(readString(pitchConfig, QStringLiteral("uvVocab"), QStringLiteral("AP, SP, br, sil")));
            int uvCond = readInt(pitchConfig, QStringLiteral("uvWordCond"), 1);
            for (int i = 0; i < uvWordCondCombo->count(); ++i) {
                if (uvWordCondCombo->itemData(i).toInt() == uvCond) {
                    uvWordCondCombo->setCurrentIndex(i);
                    break;
                }
            }
            f0MinSpin->setValue(readDouble(pitchConfig, QStringLiteral("minF0"), 50.0));
            f0MaxSpin->setValue(readDouble(pitchConfig, QStringLiteral("maxF0"), 1100.0));
        }

        const QJsonObject phNumConfig = settingsData["phNumConfig"].toObject();
        const QString specialWords = readString(phNumConfig, QStringLiteral("specialWords"));
        if (!specialWords.isEmpty())
            phNumSpecialWords->setText(specialWords);
        const QJsonObject phNumLangs = phNumConfig["languages"].toObject();
        phNumTable->setRowCount(0);
        for (auto it = phNumLangs.begin(); it != phNumLangs.end(); ++it) {
            QString lang = it.key();
            QJsonObject langCfg = it.value().toObject();
            QString dictPath_ = readString(langCfg, QStringLiteral("dictPath"));
            QString vowelsPath = readString(langCfg, QStringLiteral("vowelsPath"));
            QString consonantsPath = readString(langCfg, QStringLiteral("consonantsPath"));
            phNumTable->insertRow(phNumTable->rowCount());
            int r = phNumTable->rowCount() - 1;
            auto *langItem = new QTableWidgetItem(lang);
            phNumTable->setItem(r, 0, langItem);
            phNumTable->setCellWidget(r, 1, createPhNumPathCell());
            phNumTable->setCellWidget(r, 2, createPhNumPathCell());
            phNumTable->setCellWidget(r, 3, createPhNumPathCell());
            if (auto *le = searchLineEditInPathCell(phNumTable->cellWidget(r, 1)))
                le->setText(dictPath_);
            if (auto *le = searchLineEditInPathCell(phNumTable->cellWidget(r, 2)))
                le->setText(vowelsPath);
            if (auto *le = searchLineEditInPathCell(phNumTable->cellWidget(r, 3)))
                le->setText(consonantsPath);
        }
        if (phNumTable->rowCount() == 0) {
            phNumTable->insertRow(0);
            phNumTable->setItem(0, 0, new QTableWidgetItem(QStringLiteral("zh")));
            phNumTable->setCellWidget(0, 1, createPhNumPathCell());
            phNumTable->setCellWidget(0, 2, createPhNumPathCell());
            phNumTable->setCellWidget(0, 3, createPhNumPathCell());
        }
    }

} // namespace dstools