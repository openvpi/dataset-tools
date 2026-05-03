/// @file SettingsPage.h
/// @brief DsLabeler settings page with tabbed pipeline configuration.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/DsProject.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>

namespace dstools {

/// @brief Settings page for DsLabeler — tabbed configuration for pipeline steps.
///
/// Reads and writes DsProjectDefaults from the active DsProject.
/// Contains a unified device tab and per-model configuration tabs.
class SettingsPage : public QWidget,
                     public labeler::IPageActions,
                     public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit SettingsPage(QWidget *parent = nullptr);
    ~SettingsPage() override = default;

    /// Set the project whose defaults to edit.
    void setProject(DsProject *project);

    /// Apply current UI values to the project defaults.
    void applyToProject();

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;
    bool hasUnsavedChanges() const override;

    // IPageLifecycle
    void onActivated() override;
    bool onDeactivating() override;

signals:
    void settingsChanged();
    /// Emitted when a model needs reloading (path or device changed).
    void modelReloadRequested(const QString &modelKey);

private:
    DsProject *m_project = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    bool m_dirty = false;

    // ── Device tab widgets ──
    QComboBox *m_providerCombo = nullptr;   ///< CPU / DML / CUDA(disabled)
    QComboBox *m_deviceCombo = nullptr;     ///< Specific GPU device (name + VRAM)

    // ── ASR tab widgets ──
    QLineEdit *m_asrModelPath = nullptr;
    QCheckBox *m_asrForceCpu = nullptr;
    QPushButton *m_asrTestBtn = nullptr;

    // ── FA tab widgets ──
    QLineEdit *m_faModelPath = nullptr;
    QCheckBox *m_faForceCpu = nullptr;
    QPushButton *m_faTestBtn = nullptr;
    QCheckBox *m_faPreloadEnabled = nullptr;
    QSpinBox *m_faPreloadCount = nullptr;

    // ── Pitch/MIDI tab widgets ──
    QLineEdit *m_pitchModelPath = nullptr;
    QCheckBox *m_pitchForceCpu = nullptr;
    QPushButton *m_pitchTestBtn = nullptr;
    QLineEdit *m_midiModelPath = nullptr;
    QCheckBox *m_midiForceCpu = nullptr;
    QPushButton *m_midiTestBtn = nullptr;
    QCheckBox *m_pitchPreloadEnabled = nullptr;
    QSpinBox *m_pitchPreloadCount = nullptr;

    // ── General tab widgets ──
    QComboBox *m_languageCombo = nullptr;

    QWidget *createDeviceTab();
    QWidget *createGeneralTab();
    QWidget *createAsrTab();
    QWidget *createDictTab();
    QWidget *createFATab();
    QWidget *createPitchTab();
    QWidget *createPreprocessTab();

    QWidget *createModelConfigRow(const QString &label, QLineEdit *&pathEdit,
                                  QCheckBox *&forceCpu, QPushButton *&testBtn);

    void loadFromProject();
    void markDirty();
    void connectDirtySignals();
    void populateDeviceList();
    void onTestModel(const QString &modelKey, QLineEdit *pathEdit, QCheckBox *forceCpu);

    /// Get the effective provider for a model (considering force-CPU override).
    QString effectiveProvider(QCheckBox *forceCpu) const;
};

} // namespace dstools
