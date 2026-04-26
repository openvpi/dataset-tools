#include "ModelPathPanel.h"

#include "SettingsSerializer.h"

#include <dsfw/FileDialogHelper.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#    include <dxgi.h>
#    include <wrl/client.h>
#    pragma comment(lib, "dxgi.lib")
#endif

namespace dstools {

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

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        quint64 vram = desc.DedicatedVideoMemory;
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

ModelPathPanel::ModelPathPanel(QWidget *parent) : QWidget(parent) {}

void ModelPathPanel::populateDeviceList() {
    m_deviceCombo->clear();
    auto devices = enumerateGpuDevices();

    if (devices.isEmpty()) {
        m_deviceCombo->addItem(QStringLiteral("(无可用 GPU)"), 0);
        m_deviceCombo->setEnabled(false);
        return;
    }

    for (const auto &dev : devices) {
        double vramGB = static_cast<double>(dev.vramBytes) / (1024.0 * 1024.0 * 1024.0);
        QString label = QStringLiteral("%1 (%2 GB)").arg(dev.name).arg(vramGB, 0, 'f', 1);
        m_deviceCombo->addItem(label, dev.index);
    }
}

QWidget *ModelPathPanel::createDeviceTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    auto *group = new QGroupBox(QStringLiteral("推理设备"), w);
    auto *formLayout = new QFormLayout(group);

    m_providerCombo = new QComboBox(group);
    m_providerCombo->addItem(QStringLiteral("cpu"));
#ifdef Q_OS_WIN
    m_providerCombo->addItem(QStringLiteral("dml"));
#endif
    {
        m_providerCombo->addItem(QStringLiteral("cuda (暂不可用)"));
        auto *model = qobject_cast<QStandardItemModel *>(m_providerCombo->model());
        if (model) {
            auto *item = model->item(m_providerCombo->count() - 1);
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        }
    }
    formLayout->addRow(QStringLiteral("推理提供者:"), m_providerCombo);

    m_deviceCombo = new QComboBox(group);
    populateDeviceList();
    formLayout->addRow(QStringLiteral("设备:"), m_deviceCombo);

    layout->addWidget(group);

    auto *infoLabel = new QLabel(
        QStringLiteral("选择 CPU 或 DirectML 作为全局推理后端。\n"
                       "各模型可通过「CPU 强制」复选框单独覆盖为 CPU 运行。\n"
                       "设备列表仅显示 >=1GB 显存的 GPU。"),
        w);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-top: 8px;"));
    layout->addWidget(infoLabel);

    connect(m_providerCombo, &QComboBox::currentTextChanged, this, [this](const QString &text) {
        m_deviceCombo->setEnabled(text == QStringLiteral("dml"));
    });
    m_deviceCombo->setEnabled(m_providerCombo->currentText() == QStringLiteral("dml"));

    layout->addStretch();
    return w;
}

QWidget *ModelPathPanel::createModelConfigRow(const QString &label, QLineEdit *&pathEdit, QCheckBox *&forceCpu,
                                              QPushButton *&testBtn) {
    auto *group = new QGroupBox(label);
    auto *layout = new QVBoxLayout(group);

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

    forceCpu = new QCheckBox(QStringLiteral("CPU 强制（此模型在 CPU 上运行）"), group);
    layout->addWidget(forceCpu);

    connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit]() {
        const QString path = dsfw::FileDialogHelper::getExistingDirectory({this, QStringLiteral("选择模型目录")});
        if (!path.isEmpty())
            pathEdit->setText(path);
    });

    return group;
}

QWidget *ModelPathPanel::createOnnxModelConfigRow(const QString &label, QLineEdit *&pathEdit, QCheckBox *&forceCpu,
                                                  QPushButton *&testBtn) {
    auto *group = new QGroupBox(label);
    auto *layout = new QVBoxLayout(group);

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

    forceCpu = new QCheckBox(QStringLiteral("CPU 强制（此模型在 CPU 上运行）"), group);
    layout->addWidget(forceCpu);

    connect(browseBtn, &QPushButton::clicked, this, [this, pathEdit]() {
        const QString path =
            dsfw::FileDialogHelper::getOpenFileName({this, QStringLiteral("选择 ONNX 模型文件"), {},
                                                     {QStringLiteral("ONNX 文件 (*.onnx)"),
                                                      QStringLiteral("所有文件 (*)")}});
        if (!path.isEmpty())
            pathEdit->setText(path);
    });

    return group;
}

void ModelPathPanel::onTestModel(const QString &modelKey, QLineEdit *pathEdit, QCheckBox *forceCpu) {
    Q_UNUSED(modelKey)
    QString modelPath = pathEdit->text().trimmed();
    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试加载"), QStringLiteral("请先设置模型路径。"));
        return;
    }

    QFileInfo fileInfo(modelPath);
    bool isOnnxFile = fileInfo.isFile() && modelPath.endsWith(QStringLiteral(".onnx"), Qt::CaseInsensitive);

    if (isOnnxFile) {
        if (!fileInfo.exists()) {
            QMessageBox::critical(this, QStringLiteral("测试加载"),
                                  QStringLiteral("✗ 模型文件不存在:\n%1").arg(modelPath));
            return;
        }

        QString providerStr = SettingsSerializer::effectiveProvider(m_providerCombo, forceCpu);
        QMessageBox::information(this, QStringLiteral("测试加载"),
                                 QStringLiteral("✓ 模型文件验证通过\n\n"
                                                "文件: %1\n"
                                                "推理提供者: %2\n"
                                                "设备: %3")
                                     .arg(fileInfo.fileName())
                                     .arg(providerStr)
                                     .arg(m_deviceCombo->currentText()));
        return;
    }

    QDir dir(modelPath);
    if (!dir.exists()) {
        QMessageBox::critical(this, QStringLiteral("测试加载"),
                              QStringLiteral("✗ 模型目录不存在:\n%1").arg(modelPath));
        return;
    }

    QStringList onnxFiles = dir.entryList({QStringLiteral("*.onnx")}, QDir::Files);
    if (onnxFiles.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试加载"),
                             QStringLiteral("✗ 模型目录中没有找到 .onnx 文件:\n%1").arg(modelPath));
        return;
    }

    bool hasConfig = QFile::exists(dir.absoluteFilePath(QStringLiteral("config.json")));

    QString providerStr = SettingsSerializer::effectiveProvider(m_providerCombo, forceCpu);
    QMessageBox::information(this, QStringLiteral("测试加载"),
                             QStringLiteral("✓ 模型路径验证通过\n\n"
                                            "目录: %1\n"
                                            "ONNX 文件: %2 个\n"
                                            "config.json: %3\n"
                                            "推理提供者: %4\n"
                                            "设备: %5")
                                 .arg(modelPath)
                                 .arg(onnxFiles.size())
                                 .arg(hasConfig ? QStringLiteral("✓") : QStringLiteral("✗ (缺失)"))
                                 .arg(providerStr)
                                 .arg(m_deviceCombo->currentText()));
}

QWidget *ModelPathPanel::createAsrTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);
    layout->addWidget(createModelConfigRow(QStringLiteral("ASR 模型"), m_asrModelPath, m_asrForceCpu, m_asrTestBtn));

    connect(m_asrTestBtn, &QPushButton::clicked, this, [this]() {
        onTestModel(QStringLiteral("asr"), m_asrModelPath, m_asrForceCpu);
    });

    layout->addStretch();
    return w;
}

QWidget *ModelPathPanel::createFATab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    layout->addWidget(createModelConfigRow(QStringLiteral("强制对齐模型"), m_faModelPath, m_faForceCpu, m_faTestBtn));

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

    auto *nsGroup = new QGroupBox(QStringLiteral("非语音音素"), w);
    auto *nsLayout = new QFormLayout(nsGroup);
    m_faNonSpeechPh = new QLineEdit(nsGroup);
    m_faNonSpeechPh->setPlaceholderText(QStringLiteral("AP, SP"));
    m_faNonSpeechPh->setToolTip(QStringLiteral("逗号分隔的非语音音素列表，强制对齐时忽略这些音素"));
    nsLayout->addRow(QStringLiteral("关键字:"), m_faNonSpeechPh);
    layout->addWidget(nsGroup);

    layout->addWidget(createOnnxModelConfigRow(QStringLiteral("口型曲线模型 (R3MOE)"), m_moeModelPath, m_moeForceCpu,
                                               m_moeTestBtn));

    connect(m_moeTestBtn, &QPushButton::clicked, this, [this]() {
        onTestModel(QStringLiteral("moe_curve"), m_moeModelPath, m_moeForceCpu);
    });

    layout->addStretch();
    return w;
}

QWidget *ModelPathPanel::createPitchTab() {
    auto *w = new QWidget(this);
    auto *layout = new QVBoxLayout(w);

    layout->addWidget(createOnnxModelConfigRow(QStringLiteral("F0 提取模型 (RMVPE)"), m_pitchModelPath,
                                               m_pitchForceCpu, m_pitchTestBtn));
    layout->addWidget(createModelConfigRow(QStringLiteral("MIDI 转录模型 (GAME)"), m_midiModelPath, m_midiForceCpu,
                                           m_midiTestBtn));

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

    auto *uvGroup = new QGroupBox(QStringLiteral("GAME 对齐参数"), w);
    auto *uvLayout = new QFormLayout(uvGroup);

    m_uvVocab = new QLineEdit(uvGroup);
    m_uvVocab->setPlaceholderText(QStringLiteral("AP, SP, br, sil"));
    m_uvVocab->setToolTip(QStringLiteral("逗号分隔的无声音素词汇"));
    uvLayout->addRow(QStringLiteral("无声音素 (uvVocab):"), m_uvVocab);

    m_uvWordCondCombo = new QComboBox(uvGroup);
    m_uvWordCondCombo->addItem(QStringLiteral("Lead（前置）"), 0);
    m_uvWordCondCombo->addItem(QStringLiteral("All（全部）"), 1);
    m_uvWordCondCombo->addItem(QStringLiteral("None（无）"), 2);
    m_uvWordCondCombo->setToolTip(QStringLiteral("无声音素词汇匹配条件"));
    uvLayout->addRow(QStringLiteral("UV 词条件:"), m_uvWordCondCombo);

    layout->addWidget(uvGroup);

    auto *f0Group = new QGroupBox(QStringLiteral("F0 检测范围"), w);
    auto *f0Layout = new QFormLayout(f0Group);

    m_f0MinSpin = new QDoubleSpinBox(f0Group);
    m_f0MinSpin->setRange(10.0, 500.0);
    m_f0MinSpin->setValue(50.0);
    m_f0MinSpin->setSuffix(QStringLiteral(" Hz"));
    m_f0MinSpin->setToolTip(QStringLiteral("最小基频检测范围（Hz）"));
    f0Layout->addRow(QStringLiteral("最小 F0:"), m_f0MinSpin);

    m_f0MaxSpin = new QDoubleSpinBox(f0Group);
    m_f0MaxSpin->setRange(100.0, 2000.0);
    m_f0MaxSpin->setValue(1100.0);
    m_f0MaxSpin->setSuffix(QStringLiteral(" Hz"));
    m_f0MaxSpin->setToolTip(QStringLiteral("最大基频检测范围（Hz）"));
    f0Layout->addRow(QStringLiteral("最大 F0:"), m_f0MaxSpin);

    layout->addWidget(f0Group);

    layout->addStretch();
    return w;
}

QJsonObject ModelPathPanel::collectSettings() const {
    QJsonObject data;

    data["globalProvider"] = m_providerCombo->currentText();
    data["deviceIndex"] = m_deviceCombo->currentData().toInt();

    QJsonObject models;
    models["asr"] = SettingsSerializer::modelToJson(m_asrModelPath, m_asrForceCpu, m_providerCombo);
    models["phoneme_alignment"] = SettingsSerializer::modelToJson(m_faModelPath, m_faForceCpu, m_providerCombo);
    models["pitch_extraction"] = SettingsSerializer::modelToJson(m_pitchModelPath, m_pitchForceCpu, m_providerCombo);
    models["midi_transcription"] = SettingsSerializer::modelToJson(m_midiModelPath, m_midiForceCpu, m_providerCombo);
    models["moe_curve"] = SettingsSerializer::modelToJson(m_moeModelPath, m_moeForceCpu, m_providerCombo);
    data["taskModels"] = models;

    QJsonObject preload;
    preload["phoneme_alignment"] = SettingsSerializer::preloadToJson(m_faPreloadEnabled, m_faPreloadCount);
    preload["pitch_extraction"] = SettingsSerializer::preloadToJson(m_pitchPreloadEnabled, m_pitchPreloadCount);
    data["preload"] = preload;

    QJsonObject faConfig;
    faConfig["nonSpeechPh"] = m_faNonSpeechPh->text().trimmed().isEmpty() ? m_faNonSpeechPh->placeholderText()
                                                                          : m_faNonSpeechPh->text().trimmed();
    data["faConfig"] = faConfig;

    QJsonObject pitchConfig;
    pitchConfig["uvVocab"] =
        m_uvVocab->text().trimmed().isEmpty() ? m_uvVocab->placeholderText() : m_uvVocab->text().trimmed();
    pitchConfig["uvWordCond"] = m_uvWordCondCombo->currentData().toInt();
    pitchConfig["minF0"] = m_f0MinSpin->value();
    pitchConfig["maxF0"] = m_f0MaxSpin->value();
    data["pitchConfig"] = pitchConfig;

    return data;
}

void ModelPathPanel::applySettings(const QJsonObject &data) {
    {
        QString provider = data["globalProvider"].toString(QStringLiteral("cpu"));
        int idx = m_providerCombo->findText(provider);
        if (idx >= 0)
            m_providerCombo->setCurrentIndex(idx);

        int deviceIndex = data["deviceIndex"].toInt(0);
        for (int i = 0; i < m_deviceCombo->count(); ++i) {
            if (m_deviceCombo->itemData(i).toInt() == deviceIndex) {
                m_deviceCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    const QJsonObject models = data["taskModels"].toObject();
    SettingsSerializer::modelFromJson(models, QStringLiteral("asr"), m_asrModelPath, m_asrForceCpu);
    SettingsSerializer::modelFromJson(models, QStringLiteral("phoneme_alignment"), m_faModelPath, m_faForceCpu);
    SettingsSerializer::modelFromJson(models, QStringLiteral("pitch_extraction"), m_pitchModelPath, m_pitchForceCpu);
    SettingsSerializer::modelFromJson(models, QStringLiteral("midi_transcription"), m_midiModelPath, m_midiForceCpu);
    SettingsSerializer::modelFromJson(models, QStringLiteral("moe_curve"), m_moeModelPath, m_moeForceCpu);

    const QJsonObject preload = data["preload"].toObject();
    SettingsSerializer::preloadFromJson(preload, QStringLiteral("phoneme_alignment"), m_faPreloadEnabled,
                                        m_faPreloadCount, 10);
    SettingsSerializer::preloadFromJson(preload, QStringLiteral("pitch_extraction"), m_pitchPreloadEnabled,
                                        m_pitchPreloadCount, 10);

    const QJsonObject faConfig = data["faConfig"].toObject();
    m_faNonSpeechPh->setText(faConfig["nonSpeechPh"].toString(QStringLiteral("AP, SP")));

    const QJsonObject pitchConfig = data["pitchConfig"].toObject();
    m_uvVocab->setText(pitchConfig["uvVocab"].toString(QStringLiteral("AP, SP, br, sil")));
    int uvCond = pitchConfig["uvWordCond"].toInt(1);
    for (int i = 0; i < m_uvWordCondCombo->count(); ++i) {
        if (m_uvWordCondCombo->itemData(i).toInt() == uvCond) {
            m_uvWordCondCombo->setCurrentIndex(i);
            break;
        }
    }
    m_f0MinSpin->setValue(pitchConfig["minF0"].toDouble(50.0));
    m_f0MaxSpin->setValue(pitchConfig["maxF0"].toDouble(1100.0));
}

void ModelPathPanel::connectDirtySignals() {
    connect(m_providerCombo, &QComboBox::currentTextChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_deviceCombo, &QComboBox::currentIndexChanged, this, &ModelPathPanel::dirtyChanged);

    connect(m_asrModelPath, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_asrForceCpu, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);

    connect(m_faModelPath, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_faForceCpu, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);
    connect(m_faPreloadEnabled, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);
    connect(m_faPreloadCount, &QSpinBox::valueChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_faNonSpeechPh, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);

    connect(m_pitchModelPath, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_pitchForceCpu, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);
    connect(m_midiModelPath, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_midiForceCpu, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);
    connect(m_moeModelPath, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_moeForceCpu, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);
    connect(m_pitchPreloadEnabled, &QCheckBox::toggled, this, &ModelPathPanel::dirtyChanged);
    connect(m_pitchPreloadCount, &QSpinBox::valueChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_uvVocab, &QLineEdit::textChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_uvWordCondCombo, &QComboBox::currentIndexChanged, this, &ModelPathPanel::dirtyChanged);
    connect(m_f0MinSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ModelPathPanel::dirtyChanged);
    connect(m_f0MaxSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ModelPathPanel::dirtyChanged);
}

void ModelPathPanel::setDeviceEnabled(bool enabled) {
    if (m_deviceCombo)
        m_deviceCombo->setEnabled(enabled);
}

} // namespace dstools