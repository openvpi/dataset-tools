/// @file SettingsPage.h
/// @brief DsLabeler settings page with tabbed pipeline configuration.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/DsProject.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>

namespace dstools {

/// @brief Settings page for DsLabeler — tabbed configuration for pipeline steps.
///
/// Reads and writes DsProjectDefaults from the active DsProject.
/// Each tab corresponds to one pipeline step's configuration.
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

private:
    DsProject *m_project = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    bool m_dirty = false;

    // ── ASR tab widgets ──
    QLineEdit *m_asrModelPath = nullptr;
    QComboBox *m_asrProvider = nullptr;

    // ── FA tab widgets ──
    QLineEdit *m_faModelPath = nullptr;
    QComboBox *m_faProvider = nullptr;
    QCheckBox *m_faPreloadEnabled = nullptr;
    QSpinBox *m_faPreloadCount = nullptr;

    // ── Pitch/MIDI tab widgets ──
    QLineEdit *m_pitchModelPath = nullptr;
    QComboBox *m_pitchProvider = nullptr;
    QLineEdit *m_midiModelPath = nullptr;
    QComboBox *m_midiProvider = nullptr;
    QCheckBox *m_pitchPreloadEnabled = nullptr;
    QSpinBox *m_pitchPreloadCount = nullptr;

    // ── Export tab widgets ──
    QCheckBox *m_exportCsv = nullptr;
    QCheckBox *m_exportDs = nullptr;
    QSpinBox *m_exportHopSize = nullptr;
    QSpinBox *m_exportSampleRate = nullptr;
    QSpinBox *m_exportResampleRate = nullptr;
    QCheckBox *m_exportIncludeDiscarded = nullptr;

    // ── General tab widgets ──
    QComboBox *m_languageCombo = nullptr;

    QWidget *createGeneralTab();
    QWidget *createAsrTab();
    QWidget *createDictTab();
    QWidget *createFATab();
    QWidget *createPitchTab();
    QWidget *createExportTab();
    QWidget *createPreprocessTab();

    QWidget *createModelConfigRow(QLineEdit *&pathEdit, QComboBox *&providerCombo,
                                  const QString &label);

    void loadFromProject();
    void markDirty();
    void connectDirtySignals();
};

} // namespace dstools
