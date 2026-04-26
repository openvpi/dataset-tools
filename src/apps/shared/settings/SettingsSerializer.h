#pragma once

#include <QJsonObject>
#include <QString>
#include <functional>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class QWidget;

namespace dstools {

    class SettingsSerializer {
    public:
        static QJsonObject toJson(QComboBox *providerCombo, QComboBox *deviceCombo, QLineEdit *asrModelPath,
                                  QCheckBox *asrForceCpu, QLineEdit *faModelPath, QCheckBox *faForceCpu,
                                  QLineEdit *pitchModelPath, QCheckBox *pitchForceCpu, QLineEdit *midiModelPath,
                                  QCheckBox *midiForceCpu, QLineEdit *moeModelPath, QCheckBox *moeForceCpu,
                                  QCheckBox *faPreloadEnabled, QSpinBox *faPreloadCount, QCheckBox *pitchPreloadEnabled,
                                  QSpinBox *pitchPreloadCount, QComboBox *g2pEngineCombo, QLineEdit *dictPath,
                                  QLineEdit *faNonSpeechPh, QLineEdit *uvVocab, QComboBox *uvWordCondCombo,
                                  QDoubleSpinBox *f0MinSpin, QDoubleSpinBox *f0MaxSpin,
                                  QTableWidget *phNumTable, QLineEdit *phNumSpecialWords,
                                  const std::function<QLineEdit *(QWidget *)> &searchLineEditInPathCell);

        static void fromJson(const QJsonObject &data, QComboBox *providerCombo, QComboBox *deviceCombo,
                             QLineEdit *asrModelPath, QCheckBox *asrForceCpu, QLineEdit *faModelPath,
                             QCheckBox *faForceCpu, QLineEdit *pitchModelPath, QCheckBox *pitchForceCpu,
                             QLineEdit *midiModelPath, QCheckBox *midiForceCpu, QLineEdit *moeModelPath,
                             QCheckBox *moeForceCpu, QCheckBox *faPreloadEnabled, QSpinBox *faPreloadCount,
                             QCheckBox *pitchPreloadEnabled, QSpinBox *pitchPreloadCount, QComboBox *g2pEngineCombo,
                             QLineEdit *dictPath, QLineEdit *faNonSpeechPh, QLineEdit *uvVocab,
                             QComboBox *uvWordCondCombo, QDoubleSpinBox *f0MinSpin, QDoubleSpinBox *f0MaxSpin,
                             QTableWidget *phNumTable, QLineEdit *phNumSpecialWords,
                             const std::function<QWidget *()> &createPhNumPathCell,
                             const std::function<QLineEdit *(QWidget *)> &searchLineEditInPathCell);
        static QString effectiveProvider(QComboBox *providerCombo, QCheckBox *forceCpu);

    static QJsonObject modelToJson(QLineEdit *pathEdit, QCheckBox *forceCpu, QComboBox *providerCombo);
        static void modelFromJson(const QJsonObject &models, const QString &key, QLineEdit *pathEdit,
                                  QCheckBox *forceCpu);
        static QJsonObject preloadToJson(QCheckBox *enabledBox, QSpinBox *countSpin);
        static void preloadFromJson(const QJsonObject &preload, const QString &key, QCheckBox *enabledBox,
                                    QSpinBox *countSpin, int defaultCount);

    private:
    };

} // namespace dstools