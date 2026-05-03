#include "SettingsPage.h"

#include <QFileDialog>
#include <QDir>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include <dsfw/TranslationManager.h>
#include <dstools/OnnxEnv.h>

#ifdef Q_OS_WIN
#    include <dxgi.h>
#    include <wrl/client.h>
#    pragma comment(lib, "dxgi.lib")
#endif

namespace dstools {

// ── Device enumeration helper ────────────────────────────────────────────────

struct GpuDeviceInfo {
    QString name;
    int index = 0;
    quint64 vramBytes = 0;
};

static QVector<GpuDeviceInfo> enumerateGpuDevices() {
    QVector<GpuDeviceInfo> devices;
#ifdef Q_OS_WIN
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return devices;

    IDXGIAdapter1 *adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        adapter->Release();

        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        quint64 vram = desc.DedicatedVideoMemory;
        // Filter out devices with < 1 GB VRAM
        if (vram < 1024ULL * 1024 * 1024)
            continue;

        GpuDeviceInfo info;
        info.name = QString::fromWCharArray(desc.Description);
        info.index = static_cast<int>(i);
        info.vramBytes = vram;
        devices.append(info);
    }
#endif
    return devices;
}

// ── SettingsPage implementation ──────────────────────────────────────────────

SettingsPage::SettingsPage(QWidget *parent) : QWidget(parent) {
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabPosition(QTabWidget::North);

    m_tabWidget->addTab(createDeviceTab(), QStringLiteral("设备"));
    m_tabWidget->addTab(createAsrTab(), QStringLiteral("ASR"));
    m_tabWidget->addTab(createDictTab(), QStringLiteral("词典/G2P"));
    m_tabWidget->addTab(createFATab(), QStringLiteral("强制对齐"));
    m_tabWidget->addTab(createPitchTab(), QStringLiteral("音高/MIDI"));
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

    // Device (global provider + device index)
    defaults.globalProvider = m_providerCombo->currentText();
    defaults.deviceIndex = m_deviceCombo->currentData().toInt();

    // ASR
    {
        auto &cfg = defaults.taskModels[QStringLiteral("asr")];
        cfg.modelPath = m_asrModelPath->text();
        cfg.provider = effectiveProvider(m_asrForceCpu);
        cfg.forceCpu = m_asrForceCpu->isChecked();
    }

    // FA
    {
        auto &cfg = defaults.taskModels[QStringLiteral("phoneme_alignment")];
        cfg.modelPath = m_faModelPath->text();
        cfg.provider = effectiveProvider(m_faForceCpu);
        cfg.forceCpu = m_faForceCpu->isChecked();

        auto &pre = defaults.preload[QStringLiteral("phoneme_alignment")];
        pre.enabled = m_faPreloadEnabled->isChecked();
        pre.count = m_faPreloadCount->value();
    }

    // Pitch
    {
        auto &cfg = defaults.taskModels[QStringLiteral("pitch_extraction")];
        cfg.modelPath = m_pitchModelPath->text();
        cfg.provider = effectiveProvider(m_pitchForceCpu);
        cfg.forceCpu = m_pitchForceCpu->isChecked();
    }

    // MIDI
    {
        auto &cfg = defaults.taskModels[QStringLiteral("midi_transcription")];
        cfg.modelPath = m_midiModelPath->text();
        cfg.provider = effectiveProvider(m_midiForceCpu);
        cfg.forceCpu = m_midiForceCpu->isChecked();
    }

    // Pitch preload
    {
        auto &pre = defaults.preload[QStringLiteral("pitch_extraction")];
        pre.enabled = m_pitchPreloadEnabled->isChecked();
        pre.count = m_pitchPreloadCount->value();
    }

    m_project->setDefaults(defaults);
    m_dirty = false;
    emit settingsChanged();

    // Trigger model reloads for all models
    emit modelReloadRequested(QStringLiteral("asr"));
    emit modelReloadRequested(QStringLiteral("phoneme_alignment"));
    emit modelReloadRequested(QStringLiteral("pitch_extraction"));
    emit modelReloadRequested(QStringLiteral("midi_transcription"));
}

void SettingsPage::loadFromProject() {
    if (!m_project)
        return;

    const auto defaults = m_project->defaults();

    // Device
    {
        int idx = m_providerCombo->findText(defaults.globalProvider);
        if (idx >= 0)
            m_providerCombo->setCurrentIndex(idx);

        // Find device by index
        for (int i = 0; i < m_deviceCombo->count(); ++i) {
            if (m_deviceCombo->itemData(i).toInt() == defaults.deviceIndex) {
                m_deviceCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    // ASR
    {
        auto it = defaults.taskModels.find(QStringLiteral("asr"));
        if (it != defaults.taskModels.end()) {
            m_asrModelPath->setText(it->second.modelPath);
            m_asrForceCpu->setChecked(it->second.forceCpu);
        }
    }

    // FA
    {
        auto it = defaults.taskModels.find(QStringLiteral("phoneme_alignment"));
        if (it != defaults.taskModels.end()) {
            m_faModelPath->setText(it->second.modelPath);
            m_faForceCpu->setChecked(it->second.forceCpu);
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
            m_pitchForceCpu->setChecked(it->second.forceCpu);
        }
        auto mit = defaults.taskModels.find(QStringLiteral("midi_transcription"));
        if (mit != defaults.taskModels.end()) {
            m_midiModelPath->setText(mit->second.modelPath);
            m_midiForceCpu->setChecked(mit->second.forceCpu);
        }
        auto pit = defaults.preload.find(QStringLiteral("pitch_extraction"));
        if (pit != defaults.preload.end()) {
            m_pitchPreloadEnabled->setChecked(pit->second.enabled);
            m_pitchPreloadCount->setValue(pit->second.count);
        }
    }
}

// ── Tab creation ────────────────────────────────────────────────────────────

QWidget *SettingsPage::createDeviceTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    auto *group = new QGroupBox(QStringLiteral("推理设备"), w);
    auto *formLayout = new QFormLayout(group);

    // Provider selection
    m_providerCombo = new QComboBox(group);
    m_providerCombo->addItem(QStringLiteral("cpu"));
#ifdef Q_OS_WIN
    m_providerCombo->addItem(QStringLiteral("dml"));
#endif
    // CUDA disabled
    {
        m_providerCombo->addItem(QStringLiteral("cuda (暂不可用)"));
        auto *model = qobject_cast<QStandardItemModel *>(m_providerCombo->model());
        if (model) {
            auto *item = model->item(m_providerCombo->count() - 1);
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }
    formLayout->addRow(QStringLiteral("推理提供者:"), m_providerCombo);

    // Device selection
    m_deviceCombo = new QComboBox(group);
    populateDeviceList();
    formLayout->addRow(QStringLiteral("设备:"), m_deviceCombo);

    layout->addWidget(group);

    // Info label
    auto *infoLabel = new QLabel(
        QStringLiteral("选择 CPU 或 DirectML 作为全局推理后端。\n"
                       "各模型可通过「CPU 强制」复选框单独覆盖为 CPU 运行。\n"
                       "设备列表仅显示 ≥1GB 显存的 GPU。"),
        w);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-top: 8px;"));
    layout->addWidget(infoLabel);

    // Enable/disable device combo based on provider
    connect(m_providerCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_deviceCombo->setEnabled(text == QStringLiteral("dml"));
        markDirty();
    });
    m_deviceCombo->setEnabled(m_providerCombo->currentText() == QStringLiteral("dml"));

    layout->addStretch();
    return w;
}

void SettingsPage::populateDeviceList() {
    m_deviceCombo->clear();
    auto devices = enumerateGpuDevices();

    if (devices.isEmpty()) {
        m_deviceCombo->addItem(QStringLiteral("(无可用 GPU)"), 0);
        m_deviceCombo->setEnabled(false);
        return;
    }

    for (const auto &dev : devices) {
        double vramGB = static_cast<double>(dev.vramBytes) / (1024.0 * 1024.0 * 1024.0);
        QString label = QStringLiteral("%1 (%2 GB)")
                            .arg(dev.name)
                            .arg(vramGB, 0, 'f', 1);
        m_deviceCombo->addItem(label, dev.index);
    }
}

QWidget *SettingsPage::createModelConfigRow(const QString &label, QLineEdit *&pathEdit,
                                             QCheckBox *&forceCpu, QPushButton *&testBtn) {
    auto *group = new QGroupBox(label);
    auto *layout = new QVBoxLayout(group);

    // Path row: [path] [Browse] [Test]
    auto *pathLayout = new QHBoxLayout;
    pathEdit = new QLineEdit(group);
    auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), group);
    testBtn = new QPushButton(QStringLiteral("Test"), group);
    testBtn->setFixedWidth(50);
    testBtn->setToolTip(QStringLiteral("测试加载模型"));
    pathLayout->addWidget(pathEdit, 1);
    pathLayout->addWidget(browseBtn);
    pathLayout->addWidget(testBtn);
    layout->addLayout(pathLayout);

    // CPU override checkbox
    forceCpu = new QCheckBox(QStringLiteral("CPU 强制（此模型在 CPU 上运行）"), group);
    layout->addWidget(forceCpu);

    connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit]() {
        const QString path =
            QFileDialog::getExistingDirectory(this, QStringLiteral("选择模型目录"));
        if (!path.isEmpty())
            pathEdit->setText(path);
    });

    return group;
}

void SettingsPage::onTestModel(const QString &modelKey, QLineEdit *pathEdit, QCheckBox *forceCpu) {
    QString modelPath = pathEdit->text().trimmed();
    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试加载"),
                             QStringLiteral("请先设置模型路径。"));
        return;
    }

    // Determine provider and device
    QString providerStr = effectiveProvider(forceCpu);
    infer::ExecutionProvider provider = infer::ExecutionProvider::CPU;
    if (providerStr == QStringLiteral("dml"))
        provider = infer::ExecutionProvider::DML;
    else if (providerStr == QStringLiteral("cuda"))
        provider = infer::ExecutionProvider::CUDA;

    int deviceId = m_deviceCombo->currentData().toInt();

    // Try to create a session with any .onnx file in the directory
    QDir dir(modelPath);
    QStringList onnxFiles = dir.entryList({QStringLiteral("*.onnx")}, QDir::Files);
    if (onnxFiles.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试加载"),
                             QStringLiteral("模型目录中没有找到 .onnx 文件。"));
        return;
    }

    QString testFile = dir.absoluteFilePath(onnxFiles.first());
    std::string errorMsg;
    auto session = infer::OnnxEnv::createSession(
        testFile.toStdWString(), provider, deviceId, &errorMsg);

    if (session) {
        QMessageBox::information(this, QStringLiteral("测试加载"),
                                 QStringLiteral("✓ 模型加载成功！\n"
                                                "提供者: %1\n设备: %2")
                                     .arg(providerStr)
                                     .arg(m_deviceCombo->currentText()));
    } else {
        QMessageBox::critical(this, QStringLiteral("测试加载"),
                              QStringLiteral("✗ 模型加载失败:\n%1")
                                  .arg(QString::fromStdString(errorMsg)));
    }
}

QString SettingsPage::effectiveProvider(QCheckBox *forceCpu) const {
    if (forceCpu && forceCpu->isChecked())
        return QStringLiteral("cpu");
    return m_providerCombo->currentText().split(' ').first(); // strip "(暂不可用)"
}

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

QWidget *SettingsPage::createAsrTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);
    layout->addWidget(createModelConfigRow(QStringLiteral("ASR 模型"),
                                           m_asrModelPath, m_asrForceCpu, m_asrTestBtn));

    connect(m_asrTestBtn, &QPushButton::clicked, this, [this]() {
        onTestModel(QStringLiteral("asr"), m_asrModelPath, m_asrForceCpu);
    });

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

    layout->addWidget(createModelConfigRow(QStringLiteral("强制对齐模型"),
                                           m_faModelPath, m_faForceCpu, m_faTestBtn));

    connect(m_faTestBtn, &QPushButton::clicked, this, [this]() {
        onTestModel(QStringLiteral("phoneme_alignment"), m_faModelPath, m_faForceCpu);
    });

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

    layout->addWidget(createModelConfigRow(QStringLiteral("F0 提取模型 (RMVPE)"),
                                           m_pitchModelPath, m_pitchForceCpu, m_pitchTestBtn));
    layout->addWidget(createModelConfigRow(QStringLiteral("MIDI 转录模型 (GAME)"),
                                           m_midiModelPath, m_midiForceCpu, m_midiTestBtn));

    connect(m_pitchTestBtn, &QPushButton::clicked, this, [this]() {
        onTestModel(QStringLiteral("pitch_extraction"), m_pitchModelPath, m_pitchForceCpu);
    });
    connect(m_midiTestBtn, &QPushButton::clicked, this, [this]() {
        onTestModel(QStringLiteral("midi_transcription"), m_midiModelPath, m_midiForceCpu);
    });

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
    // Device
    connect(m_providerCombo, &QComboBox::currentTextChanged, this, &SettingsPage::markDirty);
    connect(m_deviceCombo, &QComboBox::currentIndexChanged, this, [this]() { markDirty(); });

    // ASR
    connect(m_asrModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_asrForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);

    // FA
    connect(m_faModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_faForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_faPreloadEnabled, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_faPreloadCount, &QSpinBox::valueChanged, this, [this]() { markDirty(); });

    // Pitch/MIDI
    connect(m_pitchModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_pitchForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_midiModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_midiForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_pitchPreloadEnabled, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_pitchPreloadCount, &QSpinBox::valueChanged, this, [this]() { markDirty(); });
}

} // namespace dstools
