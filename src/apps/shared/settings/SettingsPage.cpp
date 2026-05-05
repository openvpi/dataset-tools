#include "SettingsPage.h"

#include "ISettingsBackend.h"

#include <QFileDialog>
#include <QDir>
#include <QFile>
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

#include <dstools/PinyinG2PProvider.h>
#include <hubert-infer/DictionaryG2P.h>

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

static QString readString(const QJsonObject &obj, const QString &key, const QString &defaultValue = {}) {
    if (obj.contains(key) && obj[key].isString())
        return obj[key].toString();
    return defaultValue;
}

static int readInt(const QJsonObject &obj, const QString &key, int defaultValue = 0) {
    if (obj.contains(key) && obj[key].isDouble())
        return obj[key].toInt();
    return defaultValue;
}

static bool readBool(const QJsonObject &obj, const QString &key, bool defaultValue = false) {
    if (obj.contains(key) && obj[key].isBool())
        return obj[key].toBool();
    return defaultValue;
}

SettingsPage::SettingsPage(ISettingsBackend *backend, QWidget *parent) : QWidget(parent) {
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

    setBackend(backend);
}

void SettingsPage::setBackend(ISettingsBackend *backend) {
    if (m_backend)
        disconnect(m_backend, &ISettingsBackend::dataChanged, this, nullptr);

    m_backend = backend;

    if (m_backend) {
        connect(m_backend, &ISettingsBackend::dataChanged, this, [this]() {
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

    QJsonObject settingsData;

    settingsData["globalProvider"] = m_providerCombo->currentText();
    settingsData["deviceIndex"] = m_deviceCombo->currentData().toInt();

    QJsonObject models;
    {
        QJsonObject cfg;
        cfg["modelPath"] = m_asrModelPath->text();
        cfg["provider"] = effectiveProvider(m_asrForceCpu);
        cfg["forceCpu"] = m_asrForceCpu->isChecked();
        models["asr"] = cfg;
    }
    {
        QJsonObject cfg;
        cfg["modelPath"] = m_faModelPath->text();
        cfg["provider"] = effectiveProvider(m_faForceCpu);
        cfg["forceCpu"] = m_faForceCpu->isChecked();
        models["phoneme_alignment"] = cfg;
    }
    {
        QJsonObject cfg;
        cfg["modelPath"] = m_pitchModelPath->text();
        cfg["provider"] = effectiveProvider(m_pitchForceCpu);
        cfg["forceCpu"] = m_pitchForceCpu->isChecked();
        models["pitch_extraction"] = cfg;
    }
    {
        QJsonObject cfg;
        cfg["modelPath"] = m_midiModelPath->text();
        cfg["provider"] = effectiveProvider(m_midiForceCpu);
        cfg["forceCpu"] = m_midiForceCpu->isChecked();
        models["midi_transcription"] = cfg;
    }
    settingsData["taskModels"] = models;

    QJsonObject preload;
    {
        QJsonObject cfg;
        cfg["enabled"] = m_faPreloadEnabled->isChecked();
        cfg["count"] = m_faPreloadCount->value();
        preload["phoneme_alignment"] = cfg;
    }
    {
        QJsonObject cfg;
        cfg["enabled"] = m_pitchPreloadEnabled->isChecked();
        cfg["count"] = m_pitchPreloadCount->value();
        preload["pitch_extraction"] = cfg;
    }
    settingsData["preload"] = preload;

    QJsonObject g2p;
    g2p["engine"] = m_g2pEngineCombo->currentData().toString();
    g2p["dictPath"] = m_dictPath->text();
    settingsData["g2p"] = g2p;

    m_backend->save(settingsData);
    m_dirty = false;

    if (m_chartOrderList) {
        QStringList order;
        for (int i = 0; i < m_chartOrderList->count(); ++i)
            order.append(m_chartOrderList->item(i)->data(Qt::UserRole).toString());
        QSettings settings;
        settings.setValue(QStringLiteral("AudioVisualizer/chartOrder"), order.join(QLatin1Char(',')));
    }

    emit settingsChanged();

    emit modelReloadRequested(QStringLiteral("asr"));
    emit modelReloadRequested(QStringLiteral("phoneme_alignment"));
    emit modelReloadRequested(QStringLiteral("pitch_extraction"));
    emit modelReloadRequested(QStringLiteral("midi_transcription"));
}

void SettingsPage::loadFromBackend() {
    if (!m_backend)
        return;

    const QJsonObject settingsData = m_backend->load();

    {
        QString provider = readString(settingsData, "globalProvider", "cpu");
        int idx = m_providerCombo->findText(provider);
        if (idx >= 0)
            m_providerCombo->setCurrentIndex(idx);

        int deviceIndex = readInt(settingsData, "deviceIndex", 0);
        for (int i = 0; i < m_deviceCombo->count(); ++i) {
            if (m_deviceCombo->itemData(i).toInt() == deviceIndex) {
                m_deviceCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    const QJsonObject models = settingsData["taskModels"].toObject();
    {
        auto it = models.find("asr");
        if (it != models.end()) {
            auto obj = it.value().toObject();
            m_asrModelPath->setText(readString(obj, "modelPath"));
            m_asrForceCpu->setChecked(readBool(obj, "forceCpu"));
        }
    }
    {
        auto it = models.find("phoneme_alignment");
        if (it != models.end()) {
            auto obj = it.value().toObject();
            m_faModelPath->setText(readString(obj, "modelPath"));
            m_faForceCpu->setChecked(readBool(obj, "forceCpu"));
        }
    }
    {
        auto it = models.find("pitch_extraction");
        if (it != models.end()) {
            auto obj = it.value().toObject();
            m_pitchModelPath->setText(readString(obj, "modelPath"));
            m_pitchForceCpu->setChecked(readBool(obj, "forceCpu"));
        }
    }
    {
        auto it = models.find("midi_transcription");
        if (it != models.end()) {
            auto obj = it.value().toObject();
            m_midiModelPath->setText(readString(obj, "modelPath"));
            m_midiForceCpu->setChecked(readBool(obj, "forceCpu"));
        }
    }

    const QJsonObject preload = settingsData["preload"].toObject();
    {
        auto it = preload.find("phoneme_alignment");
        if (it != preload.end()) {
            auto obj = it.value().toObject();
            m_faPreloadEnabled->setChecked(readBool(obj, "enabled"));
            m_faPreloadCount->setValue(readInt(obj, "count", 10));
        }
    }
    {
        auto it = preload.find("pitch_extraction");
        if (it != preload.end()) {
            auto obj = it.value().toObject();
            m_pitchPreloadEnabled->setChecked(readBool(obj, "enabled"));
            m_pitchPreloadCount->setValue(readInt(obj, "count", 10));
        }
    }

    const QJsonObject g2p = settingsData["g2p"].toObject();
    {
        QString engine = readString(g2p, "engine", "pinyin");
        for (int i = 0; i < m_g2pEngineCombo->count(); ++i) {
            if (m_g2pEngineCombo->itemData(i).toString() == engine) {
                m_g2pEngineCombo->setCurrentIndex(i);
                break;
            }
        }
        m_dictPath->setText(readString(g2p, "dictPath"));
        m_dictPath->setEnabled(engine == QStringLiteral("dictionary"));
    }
}

QWidget *SettingsPage::createDeviceTab() {
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
                       "设备列表仅显示 ≥1GB 显存的 GPU。"),
        w);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic; margin-top: 8px;"));
    layout->addWidget(infoLabel);

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
            QFileDialog::getExistingDirectory(this, QStringLiteral("选择模型目录"));
        if (!path.isEmpty())
            pathEdit->setText(path);
    });

    return group;
}

void SettingsPage::onTestModel(const QString &modelKey, QLineEdit *pathEdit, QCheckBox *forceCpu) {
    Q_UNUSED(modelKey)
    QString modelPath = pathEdit->text().trimmed();
    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("测试加载"),
                             QStringLiteral("请先设置模型路径。"));
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

    QString providerStr = effectiveProvider(forceCpu);
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

QString SettingsPage::effectiveProvider(QCheckBox *forceCpu) const {
    if (forceCpu && forceCpu->isChecked())
        return QStringLiteral("cpu");
    return m_providerCombo->currentText().split(' ').first();
}

QWidget *SettingsPage::createGeneralTab() {
    auto *w = new QWidget(this);
    auto *layout = new QFormLayout(w);

    m_languageCombo = new QComboBox(w);
    m_languageCombo->addItem(QStringLiteral("跟随系统"), QString());
    m_languageCombo->addItem(QStringLiteral("中文 (zh_CN)"), QStringLiteral("zh_CN"));
    m_languageCombo->addItem(QStringLiteral("English (en)"), QStringLiteral("en"));

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

    layout->addItem(new QSpacerItem(0, 16, QSizePolicy::Minimum, QSizePolicy::Fixed));

    auto *chartGroup = new QGroupBox(QStringLiteral("子图显示顺序"), w);
    auto *chartLayout = new QHBoxLayout(chartGroup);

    m_chartOrderList = new QListWidget(chartGroup);

    QMap<QString, QString> chartNames = {
        {QStringLiteral("waveform"),    QStringLiteral("波形图")},
        {QStringLiteral("power"),       QStringLiteral("功率图")},
        {QStringLiteral("spectrogram"), QStringLiteral("频谱图")},
    };

    QString savedOrder = settings.value(QStringLiteral("AudioVisualizer/chartOrder")).toString();
    QStringList chartIds;
    if (!savedOrder.isEmpty()) {
        chartIds = savedOrder.split(QLatin1Char(','), Qt::SkipEmptyParts);
    } else {
        chartIds = {QStringLiteral("waveform"), QStringLiteral("power"), QStringLiteral("spectrogram")};
    }
    for (const auto &id : chartIds) {
        auto *item = new QListWidgetItem(chartNames.value(id, id), m_chartOrderList);
        item->setData(Qt::UserRole, id);
    }
    for (auto it = chartNames.constBegin(); it != chartNames.constEnd(); ++it) {
        if (!chartIds.contains(it.key())) {
            auto *item = new QListWidgetItem(it.value(), m_chartOrderList);
            item->setData(Qt::UserRole, it.key());
        }
    }

    chartLayout->addWidget(m_chartOrderList, 1);

    auto *btnLayout = new QVBoxLayout;
    m_chartUpBtn = new QPushButton(QStringLiteral("↑ 上移"), chartGroup);
    m_chartDownBtn = new QPushButton(QStringLiteral("↓ 下移"), chartGroup);
    btnLayout->addWidget(m_chartUpBtn);
    btnLayout->addWidget(m_chartDownBtn);
    btnLayout->addStretch();
    chartLayout->addLayout(btnLayout);

    layout->addRow(chartGroup);

    connect(m_chartUpBtn, &QPushButton::clicked, this, [this]() {
        int row = m_chartOrderList->currentRow();
        if (row > 0) {
            auto *item = m_chartOrderList->takeItem(row);
            m_chartOrderList->insertItem(row - 1, item);
            m_chartOrderList->setCurrentRow(row - 1);
            markDirty();
        }
    });
    connect(m_chartDownBtn, &QPushButton::clicked, this, [this]() {
        int row = m_chartOrderList->currentRow();
        if (row >= 0 && row < m_chartOrderList->count() - 1) {
            auto *item = m_chartOrderList->takeItem(row);
            m_chartOrderList->insertItem(row + 1, item);
            m_chartOrderList->setCurrentRow(row + 1);
            markDirty();
        }
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

    auto *engineGroup = new QGroupBox(QStringLiteral("G2P 引擎"), w);
    auto *engineLayout = new QFormLayout(engineGroup);

    m_g2pEngineCombo = new QComboBox(engineGroup);
    m_g2pEngineCombo->addItem(QStringLiteral("内置 Pinyin (cpp-pinyin)"), QStringLiteral("pinyin"));
    m_g2pEngineCombo->addItem(QStringLiteral("词典 G2P (DictionaryG2P)"), QStringLiteral("dictionary"));
    engineLayout->addRow(QStringLiteral("G2P 引擎:"), m_g2pEngineCombo);

    layout->addWidget(engineGroup);

    auto *dictGroup = new QGroupBox(QStringLiteral("词典路径"), w);
    auto *dictLayout = new QVBoxLayout(dictGroup);

    auto *pathLayout = new QHBoxLayout;
    m_dictPath = new QLineEdit(dictGroup);
    m_dictPath->setPlaceholderText(QStringLiteral("选择词典文件 (.txt)"));
    auto *browseBtn = new QPushButton(QStringLiteral("浏览..."), dictGroup);
    pathLayout->addWidget(m_dictPath, 1);
    pathLayout->addWidget(browseBtn);
    dictLayout->addLayout(pathLayout);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        const QString path =
            QFileDialog::getOpenFileName(this, QStringLiteral("选择词典文件"), {},
                                         QStringLiteral("词典文件 (*.txt);;所有文件 (*)"));
        if (!path.isEmpty())
            m_dictPath->setText(path);
    });

    auto *dictNote = new QLabel(
        QStringLiteral("词典格式: 每行一条，格式为「字 音」或「词 音1 音2 ...」。\n"
                       "留空则使用内置词典。"),
        dictGroup);
    dictNote->setWordWrap(true);
    dictNote->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));
    dictLayout->addWidget(dictNote);

    layout->addWidget(dictGroup);

    auto *testGroup = new QGroupBox(QStringLiteral("G2P 测试"), w);
    auto *testLayout = new QVBoxLayout(testGroup);

    auto *inputLayout = new QHBoxLayout;
    m_g2pTestInput = new QLineEdit(testGroup);
    m_g2pTestInput->setPlaceholderText(QStringLiteral("输入文本进行 G2P 转换测试"));
    m_g2pTestBtn = new QPushButton(QStringLiteral("转换"), testGroup);
    inputLayout->addWidget(m_g2pTestInput, 1);
    inputLayout->addWidget(m_g2pTestBtn);
    testLayout->addLayout(inputLayout);

    m_g2pTestResult = new QLabel(QStringLiteral("结果将显示在这里"), testGroup);
    m_g2pTestResult->setWordWrap(true);
    m_g2pTestResult->setStyleSheet(QStringLiteral("padding: 4px; background: #2a2a2a; border-radius: 3px;"));
    testLayout->addWidget(m_g2pTestResult);

    connect(m_g2pTestBtn, &QPushButton::clicked, this, [this]() {
        QString input = m_g2pTestInput->text().trimmed();
        if (input.isEmpty()) {
            m_g2pTestResult->setText(QStringLiteral("(请输入文本)"));
            return;
        }

        QString engine = m_g2pEngineCombo->currentData().toString();
        if (engine == QStringLiteral("pinyin")) {
            dstools::PinyinG2PProvider g2p;
            auto result = g2p.convert(input.toStdString(), "zh");
            if (result) {
                QStringList phonemes;
                for (const auto &r : result.value())
                    for (const auto &ph : r.phonemes)
                        phonemes << QString::fromStdString(ph);
                m_g2pTestResult->setText(phonemes.join(QStringLiteral(" ")));
            } else {
                m_g2pTestResult->setText(QStringLiteral("错误: ") + QString::fromStdString(result.error()));
            }
        } else if (engine == QStringLiteral("dictionary")) {
            QString dictPath = m_dictPath->text().trimmed();
            if (dictPath.isEmpty()) {
                m_g2pTestResult->setText(QStringLiteral("(请先设置词典路径)"));
                return;
            }
            try {
                HFA::DictionaryG2P g2p(dictPath.toStdString(), "zh");
                auto [phSeq, wordSeq, ph2word] = g2p.convert(input.toStdString(), "zh");
                QStringList phonemes;
                for (const auto &ph : phSeq)
                    phonemes << QString::fromStdString(ph);
                m_g2pTestResult->setText(phonemes.join(QStringLiteral(" ")));
            } catch (const std::exception &e) {
                m_g2pTestResult->setText(QStringLiteral("错误: ") + QString::fromLocal8Bit(e.what()));
            }
        }
    });

    layout->addWidget(testGroup);

    connect(m_g2pEngineCombo, &QComboBox::currentTextChanged, this, [this]() {
        bool isDict = m_g2pEngineCombo->currentData().toString() == QStringLiteral("dictionary");
        m_dictPath->setEnabled(isDict);
        markDirty();
    });
    m_dictPath->setEnabled(m_g2pEngineCombo->currentData().toString() == QStringLiteral("dictionary"));

    connect(m_dictPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);

    layout->addStretch();
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

QMenuBar *SettingsPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);
    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("应用设置"), this, [this]() {
        applySettings();
    });
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
}

bool SettingsPage::onDeactivating() {
    if (!m_dirty)
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的设置"),
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

void SettingsPage::connectDirtySignals() {
    connect(m_providerCombo, &QComboBox::currentTextChanged, this, &SettingsPage::markDirty);
    connect(m_deviceCombo, &QComboBox::currentIndexChanged, this, [this]() { markDirty(); });

    connect(m_asrModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_asrForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);

    connect(m_faModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_faForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_faPreloadEnabled, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_faPreloadCount, &QSpinBox::valueChanged, this, [this]() { markDirty(); });

    connect(m_pitchModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_pitchForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_midiModelPath, &QLineEdit::textChanged, this, &SettingsPage::markDirty);
    connect(m_midiForceCpu, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_pitchPreloadEnabled, &QCheckBox::toggled, this, &SettingsPage::markDirty);
    connect(m_pitchPreloadCount, &QSpinBox::valueChanged, this, [this]() { markDirty(); });
}

} // namespace dstools
