#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace dstools {

class ModelPathPanel : public QWidget {
    Q_OBJECT

public:
    explicit ModelPathPanel(QWidget *parent = nullptr);
    ~ModelPathPanel() override = default;

    QWidget *createDeviceTab();
    QWidget *createAsrTab();
    QWidget *createFATab();
    QWidget *createPitchTab();

    QJsonObject collectSettings() const;
    void applySettings(const QJsonObject &data);

    void connectDirtySignals();

    void setDeviceEnabled(bool enabled);

signals:
    void dirtyChanged();

private:
    QWidget *createModelConfigRow(const QString &label, QLineEdit *&pathEdit, QCheckBox *&forceCpu,
                                  QPushButton *&testBtn);
    QWidget *createOnnxModelConfigRow(const QString &label, QLineEdit *&pathEdit, QCheckBox *&forceCpu,
                                      QPushButton *&testBtn);
    void onTestModel(const QString &modelKey, QLineEdit *pathEdit, QCheckBox *forceCpu);
    void populateDeviceList();

    QComboBox *m_providerCombo = nullptr;
    QComboBox *m_deviceCombo = nullptr;

    QLineEdit *m_asrModelPath = nullptr;
    QCheckBox *m_asrForceCpu = nullptr;
    QPushButton *m_asrTestBtn = nullptr;

    QLineEdit *m_faModelPath = nullptr;
    QCheckBox *m_faForceCpu = nullptr;
    QPushButton *m_faTestBtn = nullptr;
    QCheckBox *m_faPreloadEnabled = nullptr;
    QSpinBox *m_faPreloadCount = nullptr;
    QLineEdit *m_faNonSpeechPh = nullptr;

    QLineEdit *m_pitchModelPath = nullptr;
    QCheckBox *m_pitchForceCpu = nullptr;
    QPushButton *m_pitchTestBtn = nullptr;
    QLineEdit *m_midiModelPath = nullptr;
    QCheckBox *m_midiForceCpu = nullptr;
    QPushButton *m_midiTestBtn = nullptr;
    QLineEdit *m_moeModelPath = nullptr;
    QCheckBox *m_moeForceCpu = nullptr;
    QPushButton *m_moeTestBtn = nullptr;
    QCheckBox *m_pitchPreloadEnabled = nullptr;
    QSpinBox *m_pitchPreloadCount = nullptr;
    QLineEdit *m_uvVocab = nullptr;
    QComboBox *m_uvWordCondCombo = nullptr;
    QDoubleSpinBox *m_f0MinSpin = nullptr;
    QDoubleSpinBox *m_f0MaxSpin = nullptr;
};

} // namespace dstools