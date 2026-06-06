#pragma once

#include "AudioSettingsPanel.h"
#include "DictionaryPanel.h"
#include "ExportSettingsPanel.h"
#include "ModelPathPanel.h"

#include <QPointer>
#include <QTabWidget>
#include <QWidget>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

namespace dstools {

class AppSettingsBackend;

class SettingsPage : public QWidget, public dsfw::IPageActions, public dsfw::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dsfw::IPageActions dsfw::IPageLifecycle)

public:
    explicit SettingsPage(AppSettingsBackend *backend, QWidget *parent = nullptr);
    ~SettingsPage() override = default;

    void setBackend(AppSettingsBackend *backend);

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
    void loadFromBackend();
    void markDirty();
    void connectDirtySignals();

    QPointer<AppSettingsBackend> m_backend;
    QTabWidget *m_tabWidget = nullptr;
    bool m_dirty = false;

    ModelPathPanel *m_modelPathPanel = nullptr;
    AudioSettingsPanel *m_audioSettingsPanel = nullptr;
    DictionaryPanel *m_dictionaryPanel = nullptr;
    ExportSettingsPanel *m_exportSettingsPanel = nullptr;
};

} // namespace dstools