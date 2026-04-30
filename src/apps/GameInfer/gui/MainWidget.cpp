#include "MainWidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <iostream>

#include "../GameInferKeys.h"
#include "../GameInferService.h"
#include <dsfw/Theme.h>

static QString replaceFileExtension(const QString &filePath, const QString &newExt) {
    const QFileInfo info(filePath);
    return info.absolutePath() + QDir::separator() + info.completeBaseName() + "." + newExt;
}

MainWidget::MainWidget(dstools::AppSettings *settings, QWidget *parent)
    : QWidget(parent), m_settings(settings), m_timeStepSeconds(0.01f), m_framesPerSecond(1.0 / 0.01) {
    m_service = new GameInferService(this);

    auto *mainLayout = new QVBoxLayout(this);

    setupModelGroup();
    setupProcessingGroup();
    setupAudioGroup();
    setupAlignGroup();
    setupActionButtons();

    mainLayout->addStretch();

    setModelLoadingStatus("Not loaded");

    max_audio_seg_length = m_settings->get(GameInferKeys::MaxAudioSegLength);

    // Automatically load config when widget is initialized
    const auto modelPath = std::filesystem::path(m_modelPath->path().toLocal8Bit().toStdString());
    if (!modelPath.empty()) {
        loadLanguagesFromConfig(modelPath);
        updateTimeStepInfo(modelPath);
    }
}

MainWidget::~MainWidget() {
    m_runningTask.cancel();
    m_runningTask.waitForFinished();
}

void MainWidget::setupModelGroup() {
    auto *group = new QGroupBox("Model Configuration");
    auto *layout = new QGridLayout(group);

    // Model path
    m_modelPath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::Directory, "Model Path:");

    const QString savedModelPath = m_settings->get(GameInferKeys::ModelPath);
    if (savedModelPath.isEmpty()) {
        m_modelPath->setPath(QApplication::applicationDirPath() + "/model/GAME-1.0.3-small-onnx");
    } else {
        m_modelPath->setPath(savedModelPath);
    }
    layout->addWidget(m_modelPath, 0, 0, 1, 5);

    connect(m_modelPath, &dstools::widgets::PathSelector::pathChanged, this, [this](const QString &dir) {
        m_settings->set(GameInferKeys::ModelPath, dir);
        if (!dir.isEmpty()) {
            loadLanguagesFromConfig(std::filesystem::path(dir.toStdWString()));
            updateTimeStepInfo(std::filesystem::path(dir.toStdWString()));
        }
    });

    // Provider selection
    layout->addWidget(new QLabel("Execution Provider:"), 1, 0);
    m_providerCombo = new QComboBox();
    m_providerCombo->addItem("CPU", static_cast<int>(Game::ExecutionProvider::CPU));
    m_providerCombo->addItem("DirectML", static_cast<int>(Game::ExecutionProvider::DML));
    layout->addWidget(m_providerCombo, 1, 1);

    // Device selection (GPU)
    layout->addWidget(new QLabel("Execution Device:"), 1, 2);
    m_deviceCombo = new dstools::widgets::GpuSelector(this);
    m_deviceCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    layout->addWidget(m_deviceCombo, 1, 3);

    // Show/hide GPU selector based on provider
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        const auto provider = static_cast<Game::ExecutionProvider>(m_providerCombo->currentData().toInt());
        m_deviceCombo->setEnabled(provider == Game::ExecutionProvider::DML);
    });

    // Status label
    m_modelStatusLabel = new QLabel("Not loaded");
    layout->addWidget(m_modelStatusLabel, 2, 0, 1, 3);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(group);
}

void MainWidget::setModelLoadingStatus(const QString &status) {
    QMetaObject::invokeMethod(
        this,
        [this, status] {
            const auto &pal = dsfw::Theme::instance().palette();
            QColor color;
            bool bold = false;
            bool italic = false;

            if (status.contains("Success") || status.contains("successfully")) {
                color = pal.success;
                bold = true;
            } else if (status.contains("Failed") || status.contains("failed") || status.contains("Error")) {
                color = pal.error;
                bold = true;
            } else if (status.contains("Loading") || status.contains("loading")) {
                color = pal.accent;
                bold = true;
            } else {
                color = pal.textSecondary;
                italic = true;
            }

            QFont f = m_modelStatusLabel->font();
            f.setBold(bold);
            f.setItalic(italic);
            m_modelStatusLabel->setFont(f);

            QPalette p = m_modelStatusLabel->palette();
            p.setColor(QPalette::WindowText, color);
            m_modelStatusLabel->setPalette(p);
            m_modelStatusLabel->setText(status);
        },
        Qt::QueuedConnection);
}


void MainWidget::setupProcessingGroup() {
    auto *group = new QGroupBox("Processing Parameters");
    auto *layout = new QGridLayout(group);

    // Row 0: Segmentation threshold
    layout->addWidget(new QLabel("分割阈值 (--seg-threshold):"), 0, 0);
    m_segThresholdSpin = new QDoubleSpinBox();
    m_segThresholdSpin->setRange(0.0, 1.0);
    m_segThresholdSpin->setSingleStep(0.01);
    m_segThresholdSpin->setValue(0.2);
    layout->addWidget(m_segThresholdSpin, 0, 1);
    m_segThresholdSpin->setValue(m_settings->get(GameInferKeys::SegThreshold));

    connect(m_segThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](const double value) { m_settings->set(GameInferKeys::SegThreshold, value); });

    // Segmentation radius in frames
    layout->addWidget(new QLabel("分割半径(帧, ms):"), 0, 2);
    m_segRadiusFrameSpin = new QSpinBox();
    m_segRadiusFrameSpin->setRange(1, 1000);
    m_segRadiusFrameSpin->setSingleStep(1);
    m_segRadiusFrameSpin->setValue(2);
    layout->addWidget(m_segRadiusFrameSpin, 0, 3);
    m_segRadiusFrameSpin->setValue(m_settings->get(GameInferKeys::SegRadiusFrame));

    connect(m_segRadiusFrameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](const int value) { m_settings->set(GameInferKeys::SegRadiusFrame, value); });

    m_segRadiusMsLabel = new QLabel("(ms)");
    layout->addWidget(m_segRadiusMsLabel, 0, 4);

    // Connect frame spin to update ms label
    connect(m_segRadiusFrameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](const int value) {
        const double ms = value * (m_timeStepSeconds * 1000.0);
        m_segRadiusMsLabel->setText(QString("(%1ms)").arg(ms, 0, 'f', 2));
    });

    // Row 1: Estimation threshold
    layout->addWidget(new QLabel("估计阈值 (--est-threshold):"), 1, 0);
    m_estThresholdSpin = new QDoubleSpinBox();
    m_estThresholdSpin->setRange(0.0, 1.0);
    m_estThresholdSpin->setSingleStep(0.01);
    m_estThresholdSpin->setValue(0.2);
    layout->addWidget(m_estThresholdSpin, 1, 1);
    m_estThresholdSpin->setValue(m_settings->get(GameInferKeys::EstThreshold));

    connect(m_estThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](const double value) { m_settings->set(GameInferKeys::EstThreshold, value); });

    // D3PM nsteps
    layout->addWidget(new QLabel("采样步数 (--seg-d3pm-nsteps):"), 1, 2);
    m_segD3PMNStepsCombo = new QComboBox();
    m_segD3PMNStepsCombo->addItem("1", 1);
    m_segD3PMNStepsCombo->addItem("2", 2);
    m_segD3PMNStepsCombo->addItem("4", 4);
    m_segD3PMNStepsCombo->addItem("8", 8);
    m_segD3PMNStepsCombo->addItem("16", 16);
    m_segD3PMNStepsCombo->setCurrentIndex(3);
    layout->addWidget(m_segD3PMNStepsCombo, 1, 3);

    m_segD3PMNStepsCombo->setCurrentIndex(m_settings->get(GameInferKeys::SegD3PMNSteps));

    // Connect D3PM parameter to update immediately
    connect(m_segD3PMNStepsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](const int value) { m_settings->set(GameInferKeys::SegD3PMNSteps, value); });

    // Row 2: Language
    layout->addWidget(new QLabel("语言ID (--language):"), 2, 0);
    m_languageCombo = new QComboBox();
    m_languageCombo->addItem("Default", 0);
    layout->addWidget(m_languageCombo, 2, 1);

    // Tempo
    layout->addWidget(new QLabel("节拍 (--tempo):"), 2, 2);
    m_tempoSpin = new QDoubleSpinBox();
    m_tempoSpin->setRange(1.0, 300.0);
    m_tempoSpin->setSingleStep(1.0);
    m_tempoSpin->setValue(120.0);
    layout->addWidget(m_tempoSpin, 2, 3);

    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(group);
}

void MainWidget::setupAudioGroup() {
    auto *group = new QGroupBox("Audio Processing");
    auto *layout = new QVBoxLayout(group);

    // WAV file selection
    m_wavPath = new dstools::widgets::PathSelector(
        dstools::widgets::PathSelector::OpenFile, "Input Audio File:",
        "Audio Files (*.wav *.flac *.mp3);;WAV Files (*.wav);;FLAC Files (*.flac);;MP3 Files (*.mp3)");
    m_wavPath->setPath(m_settings->get(GameInferKeys::WavPath));
    layout->addWidget(m_wavPath);

    const auto wavTip =
        new QLabel("Note: Mono WAV files are recommended. Multi-channel/FLAC/MP3 are for testing only.");
    layout->addWidget(wavTip);

    // Output MIDI file
    m_outputMidi = new dstools::widgets::PathSelector(
        dstools::widgets::PathSelector::SaveFile, "Output MIDI File:", "MIDI Files (*.mid)");
    m_outputMidi->setPath(m_settings->get(GameInferKeys::OutputMidiPath));
    layout->addWidget(m_outputMidi);

    // Progress bar and run button
    m_audioRun = new dstools::widgets::RunProgressRow("Convert");
    layout->addWidget(m_audioRun);

    // Connections
    connect(m_wavPath, &dstools::widgets::PathSelector::pathChanged, this, [this](const QString &wavPath) {
        m_settings->set(GameInferKeys::WavPath, wavPath);
        generateMidiOutputPath(wavPath);
    });
    connect(m_outputMidi, &dstools::widgets::PathSelector::pathChanged, this, [this](const QString &path) {
        m_settings->set(GameInferKeys::OutputMidiPath, path);
    });
    connect(m_wavPath, &dstools::widgets::PathSelector::pathChanged, this, &MainWidget::onWavPathChanged);
    connect(m_audioRun, &dstools::widgets::RunProgressRow::runClicked, this, &MainWidget::onExportMidiTask);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(group);
}

void MainWidget::setupActionButtons() {
    auto *layout = new QHBoxLayout();

    m_resetParamsBtn = new QPushButton("Reset to Defaults");
    connect(m_resetParamsBtn, &QPushButton::clicked, this, &MainWidget::resetToDefaults);
    layout->addWidget(m_resetParamsBtn);

    layout->addStretch();

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addLayout(layout);
}

void MainWidget::updateTimeStepInfo(const std::filesystem::path &modelPath) {
    m_timeStepSeconds = m_service->loadTimestepFromConfig(modelPath);
    m_framesPerSecond = 1.0 / m_timeStepSeconds;

    // Update ms label display
    const int currentValue = m_segRadiusFrameSpin->value();
    const double ms = currentValue * (m_timeStepSeconds * 1000.0);
    m_segRadiusMsLabel->setText(QString("(%1ms)").arg(ms, 0, 'f', 2));
}

bool MainWidget::loadModel(const QString &modelPathText, Game::ExecutionProvider provider, int deviceId, std::string &message) {
    if (modelPathText.isEmpty()) {
        setModelLoadingStatus("Please select model path");
        message = "Please select model path";
        return false;
    }

    std::filesystem::path modelPath = modelPathText.toLocal8Bit().toStdString();

    QMetaObject::invokeMethod(
        this, [this] { setModelLoadingStatus("Model is loading, please wait 3-10 seconds!"); }, Qt::QueuedConnection);

    std::string msg;
    bool loadSuccess = m_service->loadModel(modelPath, provider, deviceId, msg);

    if (loadSuccess) {
        QMetaObject::invokeMethod(
            this,
            [this, modelPath] {
                updateParameterValues();
                setModelLoadingStatus("Model loaded successfully!");
                m_settings->set(GameInferKeys::ModelPath, QString::fromStdWString(modelPath.wstring()));
            },
            Qt::QueuedConnection);
    } else {
        message = "Model loading failed: " + msg;
        QMetaObject::invokeMethod(
            this, [this] { setModelLoadingStatus("Model loaded failed!"); }, Qt::QueuedConnection);
        return false;
    }
    return true;
}

void MainWidget::loadLanguagesFromConfig(const std::filesystem::path &modelPath) {
    m_service->loadLanguagesFromConfig(modelPath, m_languageIdToName, m_languageNameToId);
    updateLanguageCombo();
}

void MainWidget::updateLanguageCombo() {
    // Store current selection
    const int currentId = m_languageCombo->currentData().toInt();

    // Clear and repopulate the combo box
    m_languageCombo->clear();

    // Add languages sorted by ID
    for (const auto &[id, name] : m_languageIdToName) {
        m_languageCombo->addItem(QString::fromStdString(name), id);
    }

    // Restore previous selection or default to 0
    const int index = m_languageCombo->findData(currentId);
    if (index != -1) {
        m_languageCombo->setCurrentIndex(index);
    } else {
        m_languageCombo->setCurrentIndex(0);
    }
}

void MainWidget::updateParameterValues() const {
    m_service->setSegThreshold(m_segThresholdSpin->value());
    m_service->setSegRadiusFrames(m_segRadiusFrameSpin->value());
    m_service->setEstThreshold(m_estThresholdSpin->value());
    m_service->setD3pmTimesteps(m_segD3PMNStepsCombo->currentData().toInt());
    m_service->setLanguage(m_languageCombo->currentData().toInt());
}

void MainWidget::resetToDefaults() const {
    m_segThresholdSpin->setValue(0.2);
    m_segRadiusFrameSpin->setValue(2);
    m_estThresholdSpin->setValue(0.2);
    m_segD3PMNStepsCombo->setCurrentIndex(3);
    m_languageCombo->setCurrentIndex(0);
    m_tempoSpin->setValue(120.0);

    // Reset ms label as well
    const double ms = 2 * (m_timeStepSeconds * 1000.0);
    m_segRadiusMsLabel->setText(QString("(%1ms)").arg(ms, 0, 'f', 2));
}

void MainWidget::onWavPathChanged(const QString &wavPath) const {
    if (!wavPath.isEmpty() && m_outputMidi->path().isEmpty()) {
        generateMidiOutputPath(wavPath);
    }
}

void MainWidget::generateMidiOutputPath(const QString &wavPath) const {
    const QFileInfo fileInfo(wavPath);
    const QString midiPath = replaceFileExtension(wavPath, "mid");
    m_outputMidi->setPath(midiPath);
    m_settings->set(GameInferKeys::OutputMidiPath, midiPath);
}

void MainWidget::onExportMidiTask() {
    if (m_runningTask.isRunning()) {
        QMessageBox::warning(this, "Warning", "A task is already running.");
        return;
    }

    if (m_wavPath->path().isEmpty() || m_outputMidi->path().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please fill input audio file and output MIDI file paths");
        return;
    }

    m_audioRun->setRunning(true);

    const QString wavPath = m_wavPath->path();
    const QString outputMidiPath = m_outputMidi->path();
    const float tempo = static_cast<float>(m_tempoSpin->value());
    const int maxAudioSegLength = this->max_audio_seg_length;
    const int segRadiusFrame = m_segRadiusFrameSpin->value();
    const double segThreshold = m_segThresholdSpin->value();
    const double estThreshold = m_estThresholdSpin->value();
    const int d3pmNSteps = m_segD3PMNStepsCombo->currentData().toInt();
    const int languageId = m_languageCombo->currentData().toInt();
    const QString modelPathText = m_modelPath->path();
    const auto provider = static_cast<Game::ExecutionProvider>(m_providerCombo->currentData().toInt());
    const int deviceId = m_deviceCombo->selectedDeviceId();

    m_runningTask = QtConcurrent::run([this, wavPath, outputMidiPath, tempo, maxAudioSegLength,
                                       segRadiusFrame, segThreshold, estThreshold, d3pmNSteps, languageId,
                                       modelPathText, provider, deviceId] {
        std::string msg;

        if (!exists(std::filesystem::path(wavPath.toLocal8Bit().toStdString()))) {
            QMetaObject::invokeMethod(
                this,
                [this, wavPath] {
                    QMessageBox::critical(this, "Error",
                                          QString("Audio file does not exist: %1")
                                              .arg(wavPath));
                },
                Qt::QueuedConnection);
            QMetaObject::invokeMethod(this, [this] { m_audioRun->setRunning(false); }, Qt::QueuedConnection);
            return;
        }

        if (!m_service->isModelOpen()) {
            std::string msg_;
            if (!loadModel(modelPathText, provider, deviceId, msg_)) {
                QMetaObject::invokeMethod(
                    this,
                    [this, msg_] {
                        QMessageBox::critical(this, "Error", "Model load failed! - " + QString::fromLocal8Bit(msg_));
                    },
                    Qt::QueuedConnection);
                QMetaObject::invokeMethod(this, [this] { m_audioRun->setRunning(false); }, Qt::QueuedConnection);
                return;
            }
        }

        // Set parameters on service before running
        m_service->setSegThreshold(segThreshold);
        m_service->setSegRadiusFrames(segRadiusFrame);
        m_service->setEstThreshold(estThreshold);
        m_service->setLanguage(languageId);
        m_service->setD3pmTimesteps(d3pmNSteps);

        GameInferService::MidiExportParams params;
        params.wavPath = wavPath.toLocal8Bit().toStdString();
        params.outputMidiPath = outputMidiPath.toLocal8Bit().toStdString();
        params.tempo = tempo;
        params.maxAudioSegLength = maxAudioSegLength;

        const bool success = m_service->exportMidi(params, msg, [this](const int progress) {
            QMetaObject::invokeMethod(this, [this, progress] { m_audioRun->setProgress(progress); }, Qt::QueuedConnection);
        });

        if (!success) {
            std::cerr << "Error: " << msg << std::endl;
            QMetaObject::invokeMethod(
                this,
                [this, msg] {
                    QMessageBox::critical(this, "Error", QString("Conversion failed: %1").arg(msg.c_str()));
                },
                Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(
                this, [this] { QMessageBox::information(this, "Success", "MIDI file generated!"); },
                Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(this, [this] { m_audioRun->setRunning(false); }, Qt::QueuedConnection);
    });
}

void MainWidget::setupAlignGroup() {
    auto *group = new QGroupBox("Align CSV");
    auto *layout = new QVBoxLayout(group);

    // Input CSV
    m_alignCsvInput = new dstools::widgets::PathSelector(
        dstools::widgets::PathSelector::OpenFile, "Input CSV:", "CSV Files (*.csv)");
    m_alignCsvInput->setPath(m_settings->get(GameInferKeys::AlignCsvInputPath));
    layout->addWidget(m_alignCsvInput);

    // WAV Directory
    m_alignWavDir = new dstools::widgets::PathSelector(
        dstools::widgets::PathSelector::Directory, "WAV Directory:");
    m_alignWavDir->setPath(m_settings->get(GameInferKeys::AlignWavDir));
    layout->addWidget(m_alignWavDir);

    // Output CSV
    m_alignOutput = new dstools::widgets::PathSelector(
        dstools::widgets::PathSelector::SaveFile, "Output CSV:", "CSV Files (*.csv)");
    m_alignOutput->setPath(m_settings->get(GameInferKeys::AlignOutputPath));
    layout->addWidget(m_alignOutput);

    // Progress + Run
    m_alignRun = new dstools::widgets::RunProgressRow("Align");
    layout->addWidget(m_alignRun);

    // Connections
    connect(m_alignCsvInput, &dstools::widgets::PathSelector::pathChanged, this, [this](const QString &path) {
        m_settings->set(GameInferKeys::AlignCsvInputPath, path);
    });
    connect(m_alignWavDir, &dstools::widgets::PathSelector::pathChanged, this, [this](const QString &path) {
        m_settings->set(GameInferKeys::AlignWavDir, path);
    });
    connect(m_alignOutput, &dstools::widgets::PathSelector::pathChanged, this, [this](const QString &path) {
        m_settings->set(GameInferKeys::AlignOutputPath, path);
    });
    connect(m_alignRun, &dstools::widgets::RunProgressRow::runClicked, this, &MainWidget::onAlignCsvTask);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(group);
}

void MainWidget::onAlignCsvTask() {
    if (m_runningTask.isRunning()) {
        QMessageBox::warning(this, "Warning", "A task is already running.");
        return;
    }

    if (m_alignCsvInput->path().isEmpty() || m_alignWavDir->path().isEmpty() ||
        m_alignOutput->path().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please fill all align CSV paths");
        return;
    }

    m_alignRun->setRunning(true);

    const QString csvInputPath = m_alignCsvInput->path();
    const QFileInfo outputInfo(m_alignOutput->path());
    const std::filesystem::path csvPath = csvInputPath.toLocal8Bit().toStdString();
    const std::filesystem::path savePath = outputInfo.absolutePath().toLocal8Bit().toStdString();
    const std::string saveFilename = outputInfo.fileName().toLocal8Bit().toStdString();
    const int segRadiusFrame = m_segRadiusFrameSpin->value();
    const double segThreshold = m_segThresholdSpin->value();
    const double estThreshold = m_estThresholdSpin->value();
    const int d3pmNSteps = m_segD3PMNStepsCombo->currentData().toInt();
    const int languageId = m_languageCombo->currentData().toInt();
    const QString modelPathText = m_modelPath->path();
    const auto provider = static_cast<Game::ExecutionProvider>(m_providerCombo->currentData().toInt());
    const int deviceId = m_deviceCombo->selectedDeviceId();

    m_runningTask = QtConcurrent::run([this, csvPath, savePath, saveFilename,
                                       segRadiusFrame, segThreshold, estThreshold, d3pmNSteps, languageId,
                                       modelPathText, provider, deviceId] {
        std::string msg;

        if (!m_service->isModelOpen()) {
            std::string msg_;
            if (!loadModel(modelPathText, provider, deviceId, msg_)) {
                QMetaObject::invokeMethod(
                    this,
                    [this, msg_] {
                        QMessageBox::critical(this, "Error", "Model load failed! - " + QString::fromLocal8Bit(msg_));
                    },
                    Qt::QueuedConnection);
                QMetaObject::invokeMethod(this, [this] { m_alignRun->setRunning(false); }, Qt::QueuedConnection);
                return;
            }
        }

        // Set parameters on service before running
        m_service->setSegThreshold(segThreshold);
        m_service->setSegRadiusFrames(segRadiusFrame);
        m_service->setEstThreshold(estThreshold);
        m_service->setLanguage(languageId);
        m_service->setD3pmTimesteps(d3pmNSteps);

        GameInferService::AlignCsvParams params;
        params.csvPath = csvPath;
        params.savePath = savePath;
        params.saveFilename = saveFilename;

        const bool success = m_service->alignCsv(params, msg, [this](const int progress) {
            QMetaObject::invokeMethod(this, [this, progress] { m_alignRun->setProgress(progress); },
                                     Qt::QueuedConnection);
        });

        if (!success) {
            std::cerr << "Align error: " << msg << std::endl;
            QMetaObject::invokeMethod(
                this,
                [this, msg] {
                    QMessageBox::critical(this, "Error", QString("Align failed: %1").arg(msg.c_str()));
                },
                Qt::QueuedConnection);
        } else {
            QMetaObject::invokeMethod(
                this, [this] { QMessageBox::information(this, "Success", "Align CSV completed!"); },
                Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(this, [this] { m_alignRun->setRunning(false); }, Qt::QueuedConnection);
    });
}
