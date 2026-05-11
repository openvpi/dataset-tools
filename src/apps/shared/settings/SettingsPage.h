#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QWidget>

namespace dstools {

class ISettingsBackend;

class SettingsPage : public QWidget, public labeler::IPageActions, public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit SettingsPage(ISettingsBackend *backend, QWidget *parent = nullptr);
    ~SettingsPage() override = default;

    void setBackend(ISettingsBackend *backend);

    void applySettings();

    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    void onActivated() override;
    bool onDeactivating() override;

signals:
    void settingsChanged();
    void modelReloadRequested(const QString &modelKey);

private:
    ISettingsBackend *m_backend = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    bool m_dirty = false;

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
    QCheckBox *m_pitchPreloadEnabled = nullptr;
    QSpinBox *m_pitchPreloadCount = nullptr;
    QLineEdit *m_uvVocab = nullptr;
    QComboBox *m_uvWordCondCombo = nullptr;

    QComboBox *m_languageCombo = nullptr;
    QCheckBox *m_autoSaveEnabled = nullptr;
    QSpinBox *m_autoSaveInterval = nullptr;

    QListWidget *m_chartOrderList = nullptr;
    QPushButton *m_chartUpBtn = nullptr;
    QPushButton *m_chartDownBtn = nullptr;

    QComboBox *m_g2pEngineCombo = nullptr;
    QLineEdit *m_dictPath = nullptr;
    QLineEdit *m_g2pTestInput = nullptr;
    QPushButton *m_g2pTestBtn = nullptr;
    QLabel *m_g2pTestResult = nullptr;

    QTableWidget *m_phNumTable = nullptr;
    QPushButton *m_phNumAddBtn = nullptr;
    QPushButton *m_phNumRemoveBtn = nullptr;

    QWidget *createDeviceTab();
    QWidget *createGeneralTab();
    QWidget *createAsrTab();
    QWidget *createDictTab();
    QWidget *createFATab();
    QWidget *createPitchTab();
    QWidget *createPreprocessTab();
    QWidget *createPhNumTab();

    QWidget *createPhNumPathCell();

    QLineEdit *searchLineEditInPathCell(QWidget *cellWidget) const;

    QWidget *createModelConfigRow(const QString &label, QLineEdit *&pathEdit, QCheckBox *&forceCpu,
                                  QPushButton *&testBtn);
    QWidget *createOnnxModelConfigRow(const QString &label, QLineEdit *&pathEdit, QCheckBox *&forceCpu,
                                      QPushButton *&testBtn);

    void loadFromBackend();
    void markDirty();
    void connectDirtySignals();
    void populateDeviceList();
    void onTestModel(const QString &modelKey, QLineEdit *pathEdit, QCheckBox *forceCpu);
    QString effectiveProvider(QCheckBox *forceCpu) const;
};

} // namespace dstools
