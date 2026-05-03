#include "SettingsPage.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <dsfw/TranslationManager.h>

namespace dstools {

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent) {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);

    m_tabWidget->addTab(createAsrTab(), QStringLiteral("ASR"));
    m_tabWidget->addTab(createDictTab(), QStringLiteral("词典/G2P"));
    m_tabWidget->addTab(createFATab(), QStringLiteral("强制对齐"));
    m_tabWidget->addTab(createPitchTab(), QStringLiteral("音高/MIDI"));
    m_tabWidget->addTab(createExportTab(), QStringLiteral("导出"));
    m_tabWidget->addTab(createPreprocessTab(), QStringLiteral("预处理"));
    m_tabWidget->addTab(createGeneralTab(), QStringLiteral("通用"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(m_tabWidget);

    connectDirtySignals();
}

void SettingsPage::setProject(DsProject *project) {
    m_project = project;
    loadFromProject();
    m_dirty = false;
}

void SettingsPage::applyToProject() {
    if (!m_project)
        return;

    auto defaults = m_project->defaults();

    // ASR
    {
        auto &cfg = defaults.taskModels[QStringLiteral("asr")];
        cfg.modelPath = m_asrModelPath->text();
        cfg.provider = m_asrProvider->currentText();
    }

    // FA
    {
        auto &cfg = defaults.taskModels[QStringLiteral("phoneme_alignment")];
        cfg.modelPath = m_faModelPath->text();
        cfg.provider = m_faProvider->currentText();

        auto &pre = defaults.preload[QStringLiteral("phoneme_alignment")];
        pre.enabled = m_faPreloadEnabled->isChecked();
        pre.count = m_faPreloadCount->value();
    }

    // Pitch
    {
        auto &cfg = defaults.taskModels[QStringLiteral("pitch_extraction")];
        cfg.modelPath = m_pitchModelPath->text();
        cfg.provider = m_pitchProvider->currentText();
    }

    // MIDI
    {
        auto &cfg = defaults.taskModels[QStringLiteral("midi_transcription")];
        cfg.modelPath = m_midiModelPath->text();
        cfg.provider = m_midiProvider->currentText();
    }

    // Pitch preload
    {
        auto &pre = defaults.preload[QStringLiteral("pitch_extraction")];
        pre.enabled = m_pitchPreloadEnabled->isChecked();
        pre.count = m_pitchPreloadCount->value();
    }

    // Export
    {
        QStringList formats;
        if (m_exportCsv->isChecked())
            formats << QStringLiteral("csv");
        if (m_exportDs->isChecked())
            formats << QStringLiteral("ds");
        defaults.exportConfig.formats = formats;
        defaults.exportConfig.hopSize = m_exportHopSize->value();
        defaults.exportConfig.sampleRate = m_exportSampleRate->value();
        defaults.exportConfig.resampleRate = m_exportResampleRate->value();
        defaults.exportConfig.includeDiscarded = m_exportIncludeDiscarded->isChecked();
    }

    m_project->setDefaults(defaults);
    m_dirty = false;
    emit settingsChanged();
}

void SettingsPage::loadFromProject() {
    if (!m_project)
        return;

    const auto defaults = m_project->defaults();

    // ASR
    {
        auto it = defaults.taskModels.find(QStringLiteral("asr"));
        if (it != defaults.taskModels.end()) {
            m_asrModelPath->setText(it->second.modelPath);
            m_asrProvider->setCurrentText(it->second.provider);
        }
    }

    // FA
    {
        auto it = defaults.taskModels.find(QStringLiteral("phoneme_alignment"));
        if (it != defaults.taskModels.end()) {
            m_faModelPath->setText(it->second.modelPath);
            m_faProvider->setCurrentText(it->second.provider);
        }
        auto pit = defaults.preload.find(QStringLiteral("phoneme_alignment"));
        if (pit != defaults.preload.end()) {
            m_faPreloadEnabled->setChecked(pit->second.enabled);
            m_faPreloadCount->setValue(pit->second.count);
        }
    }

    // Pitch + MIDI
    {
        auto it = defaults.taskModels.find(QStringLiteral("pitch_extraction"));
        if (it != defaults.taskModels.end()) {
            m_pitchModelPath->setText(it->second.modelPath);
            m_pitchProvider->setCurrentText(it->second.provider);
        }
        auto mit = defaults.taskModels.find(QStringLiteral("midi_transcription"));
        if (mit != defaults.taskModels.end()) {
            m_midiModelPath->setText(mit->second.modelPath);
            m_midiProvider->setCurrentText(mit->second.provider);
        }
        auto pit = defaults.preload.find(QStringLiteral("pitch_extraction"));
        if (pit != defaults.preload.end()) {
            m_pitchPreloadEnabled->setChecked(pit->second.enabled);
            m_pitchPreloadCount->setValue(pit->second.count);
        }
    }

    // Export
    {
        const auto &ec = defaults.exportConfig;
        m_exportCsv->setChecked(ec.formats.contains(QStringLiteral("csv")));
        m_exportDs->setChecked(ec.formats.contains(QStringLiteral("ds")));
        m_exportHopSize->setValue(ec.hopSize);
        m_exportSampleRate->setValue(ec.sampleRate);
        m_exportResampleRate->setValue(ec.resampleRate);
        m_exportIncludeDiscarded->setChecked(ec.includeDiscarded);
    }
}

// ── Tab creation ────────────────────────────────────────────────────────────

QWidget *SettingsPage::createGeneralTab() {
    auto *w = new QWidget(this);
    auto *layout = new QFormLayout(w);

    m_languageCombo = new QComboBox(w);
    m_languageCombo->addItem(QStringLiteral("跟随系统"), QString());
    m_languageCombo->addItem(QStringLiteral("中文 (zh_CN)"), QStringLiteral("zh_CN"));
    m_languageCombo->addItem(QStringLiteral("English (en)"), QStringLiteral("en"));

    // Load saved language
    QSettings settings;
    QString savedLang = settings.value(QStringLiteral("App/language")).toString();
    for (int i = 0; i < m_languageCombo->count(); ++i) {
        if (m_languageCombo->itemData(i).toString() == savedLang) {
            m_languageCombo->setCurrentIndex(i);
            break;
        }
    }

    layout->addRow(QStringLiteral("语言 / Language:"), m_languageCombo);

    auto *note = new QLabel(QStringLiteral("语言更改需要重启应用后生效。\n"
                                           "Language change takes effect after restart."),
                            w);
    note->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));
    layout->addRow(note);

    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this]() {
        QString lang = m_languageCombo->currentData().toString();
        QSettings settings;
        settings.setValue(QStringLiteral("App/language"), lang);
        markDirty();
    });

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
    return w;
}

QWidget *SettingsPage::createModelConfigRow(QLineEdit *&pathEdit,
                                             QComboBox *&providerCombo,
                                             const QString &label) {
    auto *group = new QGroupBox(label);
    auto *layout = new QFormLayout(group);

    pathEdit = new QLineEdit(group);
    auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), group);
    auto *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(pathEdit, 1);
    pathLayout->addWidget(browseBtn);
    layout->addRow(QStringLiteral("模型路径:"), pathLayout);

    connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit]() {
        const QString path =
            QFileDialog::getExistingDirectory(this, QStringLiteral("选择模型目录"));
        if (!path.isEmpty())
            pathEdit->setText(path);
    });

    providerCombo = new QComboBox(group);
    providerCombo->addItem(QStringLiteral("cpu"));
#ifdef Q_OS_WIN
    providerCombo->addItem(QStringLiteral("dml"));
#endif
    providerCombo->addItem(QStringLiteral("cuda"));
    layout->addRow(QStringLiteral("推理提供者:"), providerCombo);

    return group;
}

QWidget *SettingsPage::createAsrTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);
    layout->addWidget(createModelConfigRow(m_asrModelPath, m_asrProvider,
                                           QStringLiteral("ASR 模型")));
    layout->addStretch();
    return w;
}

QWidget *SettingsPage::createDictTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);
    auto *placeholder = new QLabel(
        QStringLiteral("词典配置将在后续版本实现。\n"
                       "当前使用内置 cpp-pinyin / cpp-kana 词典。"),
        w);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(QStringLiteral("color: gray;"));
    layout->addWidget(placeholder);
    return w;
}

QWidget *SettingsPage::createFATab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    layout->addWidget(createModelConfigRow(m_faModelPath, m_faProvider,
                                           QStringLiteral("强制对齐模型")));

    auto *preloadGroup = new QGroupBox(QStringLiteral("预加载"), w);
    auto *preloadLayout = new QHBoxLayout(preloadGroup);
    m_faPreloadEnabled = new QCheckBox(QStringLiteral("启用预加载"), preloadGroup);
    m_faPreloadCount = new QSpinBox(preloadGroup);
    m_faPreloadCount->setRange(1, 100);
    m_faPreloadCount->setValue(10);
    m_faPreloadCount->setPrefix(QStringLiteral("文件数: "));
    preloadLayout->addWidget(m_faPreloadEnabled);
    preloadLayout->addWidget(m_faPreloadCount);
    preloadLayout->addStretch();
    layout->addWidget(preloadGroup);

    layout->addStretch();
    return w;
}

QWidget *SettingsPage::createPitchTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    layout->addWidget(createModelConfigRow(m_pitchModelPath, m_pitchProvider,
                                           QStringLiteral("F0 提取模型 (RMVPE)")));
    layout->addWidget(createModelConfigRow(m_midiModelPath, m_midiProvider,
                                           QStringLiteral("MIDI 转录模型 (GAME)")));

    auto *preloadGroup = new QGroupBox(QStringLiteral("预加载"), w);
    auto *preloadLayout = new QHBoxLayout(preloadGroup);
    m_pitchPreloadEnabled = new QCheckBox(QStringLiteral("启用预加载"), preloadGroup);
    m_pitchPreloadCount = new QSpinBox(preloadGroup);
    m_pitchPreloadCount->setRange(1, 100);
    m_pitchPreloadCount->setValue(10);
    m_pitchPreloadCount->setPrefix(QStringLiteral("文件数: "));
    preloadLayout->addWidget(m_pitchPreloadEnabled);
    preloadLayout->addWidget(m_pitchPreloadCount);
    preloadLayout->addStretch();
    layout->addWidget(preloadGroup);

    layout->addStretch();
    return w;
}

QWidget *SettingsPage::createExportTab() {
    auto *w = new QWidget(this);
    auto *layout = new QFormLayout(w);

    auto *fmtGroup = new QGroupBox(QStringLiteral("导出格式"), w);
    auto *fmtLayout = new QVBoxLayout(fmtGroup);
    m_exportCsv = new QCheckBox(QStringLiteral("transcriptions.csv"), fmtGroup);
    m_exportCsv->setChecked(true);
    m_exportDs = new QCheckBox(QStringLiteral("ds/ 文件夹 (.ds 训练文件)"), fmtGroup);
    m_exportDs->setChecked(true);
    fmtLayout->addWidget(m_exportCsv);
    fmtLayout->addWidget(m_exportDs);
    layout->addRow(fmtGroup);

    m_exportHopSize = new QSpinBox(w);
    m_exportHopSize->setRange(64, 2048);
    m_exportHopSize->setValue(512);
    layout->addRow(QStringLiteral("hop_size:"), m_exportHopSize);

    m_exportSampleRate = new QSpinBox(w);
    m_exportSampleRate->setRange(8000, 96000);
    m_exportSampleRate->setValue(44100);
    m_exportSampleRate->setSuffix(QStringLiteral(" Hz"));
    layout->addRow(QStringLiteral("采样率:"), m_exportSampleRate);

    m_exportResampleRate = new QSpinBox(w);
    m_exportResampleRate->setRange(8000, 96000);
    m_exportResampleRate->setValue(44100);
    m_exportResampleRate->setSuffix(QStringLiteral(" Hz"));
    layout->addRow(QStringLiteral("重采样率:"), m_exportResampleRate);

    m_exportIncludeDiscarded = new QCheckBox(QStringLiteral("包含丢弃的切片"), w);
    layout->addRow(m_exportIncludeDiscarded);

    return w;
}

QWidget *SettingsPage::createPreprocessTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);
    auto *placeholder = new QLabel(
        QStringLiteral("预处理配置将在后续版本实现。\n"
                       "计划支持: 响度归一化、降噪。"),
        w);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet(QStringLiteral("color: gray;"));
    layout->addWidget(placeholder);
    return w;
}

// ── IPageActions / IPageLifecycle ────────────────────────────────────────────

QMenuBar *SettingsPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);
    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("应用设置"), this, [this]() {
        applyToProject();
    });
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window())
            w->close();
    });
    return bar;
}

QString SettingsPage::windowTitle() const {
    return QStringLiteral("DsLabeler — 设置");
}

bool SettingsPage::hasUnsavedChanges() const {
    return m_dirty;
}

void SettingsPage::onActivated() {
    loadFromProject();
}

bool SettingsPage::onDeactivating() {
    if (!m_dirty)
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的设置"),
        QStringLiteral("设置已修改，是否应用？"),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (ret == QMessageBox::Yes) {
        applyToProject();
        return true;
    }
    if (ret == QMessageBox::No) {
        m_dirty = false;
        return true;
    }
    return false; // Cancel
}

void SettingsPage::markDirty() {
    m_dirty = true;
}

void SettingsPage::connectDirtySignals() {
    // ASR
    connect(m_asrModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_asrProvider, &QComboBox::currentTextChanged, this, &SettingsPage::markDirty);

    // FA
    connect(m_faModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_faProvider, &QComboBox::currentTextChanged, this, &SettingsPage::markDirty);
    connect(m_faPreloadEnabled, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_faPreloadCount, &QSpinBox::valueChanged, this, &SettingsPage::markDirty);

    // Pitch/MIDI
    connect(m_pitchModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_pitchProvider, &QComboBox::currentTextChanged, this, &SettingsPage::markDirty);
    connect(m_midiModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_midiProvider, &QComboBox::currentTextChanged, this, &SettingsPage::markDirty);
    connect(m_pitchPreloadEnabled, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_pitchPreloadCount, &QSpinBox::valueChanged, this, &SettingsPage::markDirty);

    // Export
    connect(m_exportCsv, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_exportDs, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_exportHopSize, &QSpinBox::valueChanged, this, &SettingsPage::markDirty);
    connect(m_exportSampleRate, &QSpinBox::valueChanged, this, &SettingsPage::markDirty);
    connect(m_exportResampleRate, &QSpinBox::valueChanged, this, &SettingsPage::markDirty);
    connect(m_exportIncludeDiscarded, &QCheckBox::toggled, this, &SettingsPage::markDirty);
}

} // namespace dstools
