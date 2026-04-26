#include "SettingsPage.h"

#include "AppSettingsBackend.h"

#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>

namespace dstools {

SettingsPage::SettingsPage(AppSettingsBackend *backend, QWidget *parent) : QWidget(parent) {
    m_modelPathPanel = new ModelPathPanel(this);
    m_audioSettingsPanel = new AudioSettingsPanel(this);
    m_dictionaryPanel = new DictionaryPanel(this);
    m_exportSettingsPanel = new ExportSettingsPanel(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);

    m_tabWidget->addTab(m_modelPathPanel->createDeviceTab(), QStringLiteral("设备"));
    m_tabWidget->addTab(m_audioSettingsPanel->createAudioTab(), QStringLiteral("音频"));
    m_tabWidget->addTab(m_modelPathPanel->createAsrTab(), QStringLiteral("ASR"));
    m_tabWidget->addTab(m_dictionaryPanel->createDictTab(), QStringLiteral("词典/G2P"));
    m_tabWidget->addTab(m_modelPathPanel->createFATab(), QStringLiteral("强制对齐"));
    m_tabWidget->addTab(m_modelPathPanel->createPitchTab(), QStringLiteral("音高/MIDI"));
    m_tabWidget->addTab(m_dictionaryPanel->createPhNumTab(), QStringLiteral("ph_num"));
    m_tabWidget->addTab(m_exportSettingsPanel->createPreprocessTab(), QStringLiteral("预处理"));
    m_tabWidget->addTab(m_exportSettingsPanel->createDisplayTab(), QStringLiteral("显示"));
    m_tabWidget->addTab(m_exportSettingsPanel->createGeneralTab(), QStringLiteral("通用"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_tabWidget);

    connectDirtySignals();

    setBackend(backend);
}

void SettingsPage::setBackend(AppSettingsBackend *backend) {
    if (m_backend)
        disconnect(m_backend, &AppSettingsBackend::dataChanged, this, nullptr);

    m_backend = backend;

    if (m_backend) {
        connect(m_backend, &AppSettingsBackend::dataChanged, this, [this]() {
            loadFromBackend();
            m_dirty = false;
        });
        loadFromBackend();
    }

    m_dirty = false;
}

void SettingsPage::applySettings() {
    if (!m_backend)
        return;

    QJsonObject data;
    {
        QJsonObject modelData = m_modelPathPanel->collectSettings();
        for (auto it = modelData.begin(); it != modelData.end(); ++it)
            data[it.key()] = it.value();
    }
    {
        QJsonObject dictData = m_dictionaryPanel->collectSettings();
        for (auto it = dictData.begin(); it != dictData.end(); ++it)
            data[it.key()] = it.value();
    }

    m_backend->save(data);
    m_dirty = false;

    m_exportSettingsPanel->collectAndSaveAutoSave();
    m_exportSettingsPanel->collectAndSaveChartLayout();
    m_exportSettingsPanel->collectAndSaveLanguage();
    m_exportSettingsPanel->collectAndSaveDefaultResolution();

    emit settingsChanged();

    emit modelReloadRequested(QStringLiteral("asr"));
    emit modelReloadRequested(QStringLiteral("phoneme_alignment"));
    emit modelReloadRequested(QStringLiteral("pitch_extraction"));
    emit modelReloadRequested(QStringLiteral("midi_transcription"));
    emit modelReloadRequested(QStringLiteral("moe_curve"));
}

void SettingsPage::loadFromBackend() {
    if (!m_backend)
        return;

    const QJsonObject settingsData = m_backend->load();

    m_modelPathPanel->applySettings(settingsData);
    m_dictionaryPanel->applySettings(settingsData);
}

void SettingsPage::connectDirtySignals() {
    connect(m_modelPathPanel, &ModelPathPanel::dirtyChanged, this, &SettingsPage::markDirty);
    connect(m_audioSettingsPanel, &AudioSettingsPanel::dirtyChanged, this, &SettingsPage::markDirty);
    connect(m_dictionaryPanel, &DictionaryPanel::dirtyChanged, this, &SettingsPage::markDirty);
    connect(m_exportSettingsPanel, &ExportSettingsPanel::dirtyChanged, this, &SettingsPage::markDirty);

    m_modelPathPanel->connectDirtySignals();
    m_audioSettingsPanel->connectDirtySignals();
    m_dictionaryPanel->connectDirtySignals();
    m_exportSettingsPanel->connectDirtySignals();
}

QMenuBar *SettingsPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);
    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("应用设置"), this, [this]() { applySettings(); });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window())
            w->close();
    });
    return bar;
}

QString SettingsPage::windowTitle() const {
    return QStringLiteral("设置");
}

bool SettingsPage::hasUnsavedChanges() const {
    return m_dirty;
}

void SettingsPage::onActivated() {
    loadFromBackend();
    m_dirty = false;
}

bool SettingsPage::onDeactivating() {
    if (!m_dirty)
        return true;

    auto ret = QMessageBox::question(this, QStringLiteral("未保存的设置"),
                                     QStringLiteral("设置已修改，是否应用？"),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (ret == QMessageBox::Yes) {
        applySettings();
        return true;
    }
    if (ret == QMessageBox::No) {
        m_dirty = false;
        return true;
    }
    return false;
}

void SettingsPage::markDirty() {
    m_dirty = true;
}

} // namespace dstools