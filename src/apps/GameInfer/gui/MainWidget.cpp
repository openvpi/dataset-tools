#include "MainWidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent/QtConcurrentRun>
#include <fstream>
#include <iostream>

#include <wolf-midi/MidiFile.h>

#include "utils/DmlGpuUtils.h"

static void makeMidiFile(const std::filesystem::path &midi_path, std::vector<Game::GameMidi> midis, const float tempo) {
    Midi::MidiFile midi;
    midi.setFileFormat(1);
    midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
    midi.setResolution(480);

    midi.createTrack();

    midi.createTimeSignatureEvent(0, 0, 4, 4);
    midi.createTempoEvent(0, 0, tempo);

    std::vector<char> trackName;
    std::string str = "Game";
    trackName.insert(trackName.end(), str.begin(), str.end());

    midi.createTrack();
    midi.createMetaEvent(1, 0, Midi::MidiEvent::MetaNumbers::TrackName, trackName);

    for (const auto &[note, start, duration] : midis) {
        midi.createNoteOnEvent(1, start, 0, note, 64);
        midi.createNoteOffEvent(1, start + duration, 0, note, 64);
    }

    midi.save(midi_path);
}

MainWidget::MainWidget(QSettings *settings, QWidget *parent)
    : QWidget(parent), m_settings(settings), m_timeStepSeconds(0.02322), m_framesPerSecond(1.0 / 0.02322),
      m_isLoading(false) {
    auto *mainLayout = new QVBoxLayout(this);

    setupModelGroup();
    setupProcessingGroup();
    setupAudioGroup();
    setupActionButtons();

    mainLayout->addStretch();

    setModelLoadingStatus("Not loaded", false);
}

void MainWidget::setupModelGroup() {
    auto *group = new QGroupBox("Model Configuration");
    auto *layout = new QGridLayout(group);

    // Model path
    layout->addWidget(new QLabel("Model Path:"), 0, 0);
    m_modelPathEdit = new QLineEdit();

    const QString savedModelPath = m_settings->value("MainWidget/modelPath", "").toString();
    if (savedModelPath.isEmpty()) {
        m_modelPathEdit->setText(QApplication::applicationDirPath() + "/model/GAME-1.0-small-onnx");
    } else {
        m_modelPathEdit->setText(savedModelPath);
    }
    layout->addWidget(m_modelPathEdit, 0, 1, 1, 3);

    m_browseModelBtn = new QPushButton("Browse...");
    connect(m_browseModelBtn, &QPushButton::clicked, this, &MainWidget::browseModelPath);
    layout->addWidget(m_browseModelBtn, 0, 4);

    // Provider selection
    layout->addWidget(new QLabel("Execution Provider:"), 1, 0);
    m_providerCombo = new QComboBox();
    m_providerCombo->addItem("CPU", static_cast<int>(Game::ExecutionProvider::CPU));
    m_providerCombo->addItem("DirectML", static_cast<int>(Game::ExecutionProvider::DML));
    layout->addWidget(m_providerCombo, 1, 1);

    // Device selection
    layout->addWidget(new QLabel("Execution Device:"), 1, 2);
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_deviceCombo->addItem("Default (CPU)", -1);
    layout->addWidget(m_deviceCombo, 1, 3);

    // Refresh devices when provider changes
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateDeviceList(); });

    // Status label and Load model button in last row
    m_modelStatusLabel = new QLabel("Not loaded");
    m_modelStatusLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    layout->addWidget(m_modelStatusLabel, 2, 0, 1, 3);

    m_loadModelBtn = new QPushButton("Load Model");
    connect(m_loadModelBtn, &QPushButton::clicked, this, &MainWidget::loadModel);
    layout->addWidget(m_loadModelBtn, 2, 4);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(group);
}

void MainWidget::setModelLoadingStatus(const QString &status, const bool isLoading) {
    m_isLoading = isLoading;

    if (isLoading) {
        m_modelStatusLabel->setText(status);
        m_modelStatusLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
        m_loadModelBtn->setEnabled(false);
    } else {
        if (status.contains("Success") || status.contains("successfully")) {
            m_modelStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        } else if (status.contains("Failed") || status.contains("Error")) {
            m_modelStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        } else {
            m_modelStatusLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
        }

        m_modelStatusLabel->setText(status);
        m_loadModelBtn->setEnabled(true);
    }
}

void MainWidget::updateDeviceList() const {
    m_deviceCombo->clear();

    const auto provider = static_cast<Game::ExecutionProvider>(m_providerCombo->currentData().toInt());

    if (provider == Game::ExecutionProvider::DML) {
#ifdef _WIN32
        QList<GpuInfo> gpuList = DmlGpuUtils::getGpuList();
        for (const auto &gpu : gpuList) {
            const double memoryGB = static_cast<double>(gpu.memory) / (1024 * 1024 * 1024);
            QString deviceName = QString("%1 (%2 GB)").arg(gpu.description).arg(memoryGB, 0, 'f', 2);
            m_deviceCombo->addItem(deviceName, gpu.index);
        }
#endif
    }
    m_deviceCombo->addItem("Default", -1);
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

    // Segmentation radius in frames
    layout->addWidget(new QLabel("分割半径(帧, ms):"), 0, 2);
    m_segRadiusFrameSpin = new QSpinBox();
    m_segRadiusFrameSpin->setRange(1, 1000);
    m_segRadiusFrameSpin->setSingleStep(1);
    m_segRadiusFrameSpin->setValue(2);
    layout->addWidget(m_segRadiusFrameSpin, 0, 3);

    m_segRadiusMsLabel = new QLabel("(ms)");
    layout->addWidget(m_segRadiusMsLabel, 0, 4);

    // Connect frame spin to update ms label
    connect(m_segRadiusFrameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](const int value) {
        const double ms = value * (m_timeStepSeconds * 1000.0);
        m_segRadiusMsLabel->setText(QString("(%1ms)").arg(ms, 0, 'f', 2));
    });

    // Connect segmentation parameters to update immediately
    connect(m_segThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](const double value) {
        if (m_game) {
            m_game->set_seg_threshold(value);
        }
    });
    connect(m_segRadiusFrameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](const int value) {
        if (m_game) {
            m_game->set_seg_radius_frames(value);
        }
    });

    // Row 1: Estimation threshold
    layout->addWidget(new QLabel("估计阈值 (--est-threshold):"), 1, 0);
    m_estThresholdSpin = new QDoubleSpinBox();
    m_estThresholdSpin->setRange(0.0, 1.0);
    m_estThresholdSpin->setSingleStep(0.01);
    m_estThresholdSpin->setValue(0.2);
    layout->addWidget(m_estThresholdSpin, 1, 1);

    // Connect estimation parameter to update immediately
    connect(m_estThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](const double value) {
        if (m_game) {
            m_game->set_est_threshold(value);
        }
    });

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

    // Connect D3PM parameter to update immediately
    connect(m_segD3PMNStepsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (m_game) {
            const int nSteps = m_segD3PMNStepsCombo->currentData().toInt();
            std::vector<float> generatedTs;

            if (nSteps > 0) {
                constexpr float t0 = 0.0f;
                const float step = (1.0f - t0) / nSteps;
                for (int i = 0; i < nSteps; ++i) {
                    generatedTs.push_back(t0 + i * step);
                }
            }
            m_game->set_d3pm_ts(generatedTs);
        }
    });

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

    // Connect language parameter to update immediately
    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        if (m_game) {
            const int languageId = m_languageCombo->currentData().toInt();
            m_game->set_language(languageId);
        }
    });

    connect(m_tempoSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](const double value) { m_settings->setValue("MainWidget/tempo", value); });

    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(group);
}

void MainWidget::setupAudioGroup() {
    auto *group = new QGroupBox("Audio Processing");
    auto *layout = new QVBoxLayout(group);

    // WAV file selection
    const auto wavPathLayout = new QGridLayout();
    const auto wavPathLabel = new QLabel("Input Audio File:");
    m_wavPathLineEdit = new QLineEdit();
    m_wavPathLineEdit->setText(m_settings->value("MainWidget/wavPath", "").toString());
    m_wavPathButton = new QPushButton("Browse...");
    wavPathLayout->addWidget(wavPathLabel, 0, 0);
    wavPathLayout->addWidget(m_wavPathLineEdit, 0, 1);
    wavPathLayout->addWidget(m_wavPathButton, 0, 2);
    layout->addLayout(wavPathLayout);

    const auto wavTip =
        new QLabel("Note: Mono WAV files are recommended. Multi-channel/FLAC/MP3 are for testing only.");
    layout->addWidget(wavTip);

    // Output MIDI file
    const auto outputMidiLayout = new QGridLayout();
    const auto outputMidiLabel = new QLabel("Output MIDI File:");
    m_outputMidiLineEdit = new QLineEdit();
    m_outputMidiLineEdit->setText(m_settings->value("MainWidget/outMidiPath", "").toString());
    m_outputMidiButton = new QPushButton("Browse...");
    outputMidiLayout->addWidget(outputMidiLabel, 0, 0);
    outputMidiLayout->addWidget(m_outputMidiLineEdit, 0, 1);
    outputMidiLayout->addWidget(m_outputMidiButton, 0, 2);
    layout->addLayout(outputMidiLayout);

    // Progress bar and run button
    const auto progressLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    progressLayout->addWidget(m_progressBar);
    m_runButton = new QPushButton("Convert");
    progressLayout->addWidget(m_runButton);
    layout->addLayout(progressLayout);

    // Connections
    connect(m_wavPathButton, &QPushButton::clicked, this, &MainWidget::onBrowseWavPath);
    connect(m_outputMidiButton, &QPushButton::clicked, this, &MainWidget::onBrowseOutputMidi);
    connect(m_wavPathLineEdit, &QLineEdit::textChanged, this, &MainWidget::onWavPathChanged);
    connect(m_runButton, &QPushButton::clicked, this, &MainWidget::onExportMidiTask);

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

void MainWidget::browseModelPath() {
    if (m_isLoading) {
        QMessageBox::information(this, "Info", "Model is loading, please wait...");
        return;
    }

    const QString dir = QFileDialog::getExistingDirectory(this, "Select Model Directory", m_modelPathEdit->text(),
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_modelPathEdit->setText(dir);
        m_settings->setValue("MainWidget/modelPath", dir);
    }
}

void MainWidget::updateTimeStepInfo(const std::filesystem::path &modelPath) {
    const std::filesystem::path configPath = modelPath / "config.json";
    std::ifstream configFile(configPath);

    if (configFile.is_open()) {
        try {
            nlohmann::json config;
            configFile >> config;

            if (config.contains("timestep")) {
                if (config["timestep"].is_number_float()) {
                    m_timeStepSeconds = config["timestep"].get<float>();
                } else if (config["timestep"].is_string()) {
                    const auto timestepStr = config["timestep"].get<std::string>();
                    m_timeStepSeconds = std::stof(timestepStr);
                }

                if (m_timeStepSeconds > 0) {
                    m_framesPerSecond = 1.0 / m_timeStepSeconds;
                } else {
                    m_timeStepSeconds = 0.01;
                    m_framesPerSecond = 1.0 / 0.01;
                }
            } else {
                m_timeStepSeconds = 0.01;
                m_framesPerSecond = 1.0 / 0.01;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error parsing config.json for timestep: " << e.what() << std::endl;
            m_timeStepSeconds = 0.01;
            m_framesPerSecond = 1.0 / 0.01;
        }

        configFile.close();
    } else {
        m_timeStepSeconds = 0.02322;
        m_framesPerSecond = 1.0 / 0.02322;
    }

    // Update ms label display
    const int currentValue = m_segRadiusFrameSpin->value();
    const double ms = currentValue * (m_timeStepSeconds * 1000.0);
    m_segRadiusMsLabel->setText(QString("(%1ms)").arg(ms, 0, 'f', 2));
}

void MainWidget::loadModel() {
    if (m_isLoading) {
        QMessageBox::information(this, "Info", "Model is already loading, please wait...");
        return;
    }

    if (m_modelPathEdit->text().isEmpty()) {
        setModelLoadingStatus("Please select model path", false);
        QMessageBox::warning(this, "Warning", "Please select model path");
        return;
    }

    setModelLoadingStatus("Loading model, please wait a few seconds...", true);

    const auto res = QtConcurrent::run([this] {
        try {
            std::filesystem::path modelPath = m_modelPathEdit->text().toStdWString();

            // Get execution provider from combo box
            auto provider = static_cast<Game::ExecutionProvider>(m_providerCombo->currentData().toInt());

            int deviceId = m_deviceCombo->currentData().toInt();

            // Create new instance
            auto game = std::make_shared<Game::Game>(modelPath, provider, deviceId);

            if (game->is_open()) {
                // Successfully loaded, update UI
                QMetaObject::invokeMethod(
                    this,
                    [this, game, modelPath] {
                        m_game = game;

                        // Load timestep info from config after successful model loading
                        updateTimeStepInfo(modelPath);

                        // Load languages from config after successful model loading
                        loadLanguagesFromConfig(modelPath);

                        setModelLoadingStatus("Model loaded successfully!", false);
                        m_settings->setValue("MainWidget/modelPath", QString::fromStdString(modelPath.string()));

                        updateParameterValues();
                    },
                    Qt::QueuedConnection);
            } else {
                QMetaObject::invokeMethod(
                    this,
                    [this] {
                        setModelLoadingStatus("Model loading failed!", false);
                        QMessageBox::critical(this, "Error", "Model loading failed!");
                    },
                    Qt::QueuedConnection);
            }
        } catch (const std::exception &e) {
            QMetaObject::invokeMethod(
                this,
                [this, e] {
                    setModelLoadingStatus("Error loading model: " + QString(e.what()), false);
                    QMessageBox::critical(this, "Error", QString("Error loading model: %1").arg(e.what()));
                },
                Qt::QueuedConnection);
        }
    });
}

void MainWidget::loadLanguagesFromConfig(const std::filesystem::path &modelPath) {
    // Clear existing mappings
    m_languageIdToName.clear();
    m_languageNameToId.clear();

    // Add default option
    m_languageIdToName[0] = "default";
    m_languageNameToId["default"] = 0;

    // Try to load languages from config.json
    const std::filesystem::path configPath = modelPath / "config.json";
    std::ifstream configFile(configPath);

    if (configFile.is_open()) {
        try {
            nlohmann::json config;
            configFile >> config;

            if (config.contains("languages") && config["languages"].is_object()) {
                for (auto &[key, value] : config["languages"].items()) {
                    if (value.is_number_integer()) {
                        int id = value.get<int>();
                        m_languageIdToName[id] = key;
                        m_languageNameToId[key] = id;
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error parsing config.json: " << e.what() << std::endl;
        }

        configFile.close();
    }

    // Update the language combo box
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

void MainWidget::updateParameterValues() {
    if (!m_game) {
        return;
    }

    try {
        // Update segmentation threshold
        m_game->set_seg_threshold(m_segThresholdSpin->value());

        // Update segmentation radius in frames
        m_game->set_seg_radius_frames(m_segRadiusFrameSpin->value());

        // Update estimation threshold
        m_game->set_est_threshold(m_estThresholdSpin->value());

        // Update D3PM nsteps (generate time steps automatically with t0=0)
        const int nSteps = m_segD3PMNStepsCombo->currentData().toInt();
        std::vector<float> generatedTs;

        if (nSteps > 0) {
            constexpr float t0 = 0.0f;
            const float step = (1.0f - t0) / nSteps;
            for (int i = 0; i < nSteps; ++i) {
                generatedTs.push_back(t0 + i * step);
            }
        }
        m_game->set_d3pm_ts(generatedTs);

        // Update language - get the selected ID from the combo box
        const int languageId = m_languageCombo->currentData().toInt();
        m_game->set_language(languageId);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Error updating parameters: %1").arg(e.what()));
    }
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

void MainWidget::onBrowseWavPath() {
    if (m_isLoading) {
        QMessageBox::information(this, "Info", "Model is loading, please wait...");
        return;
    }

    const QString wavPath = QFileDialog::getOpenFileName(
        this, "Select Input Audio File", "",
        "Audio Files (*.wav *.flac *.mp3);;WAV Files (*.wav);;FLAC Files (*.flac);;MP3 Files (*.mp3)");
    if (!wavPath.isEmpty()) {
        m_wavPathLineEdit->setText(wavPath);
        m_settings->setValue("MainWidget/wavPath", wavPath);
        generateMidiOutputPath(wavPath);
    }
}

void MainWidget::onBrowseOutputMidi() {
    if (m_isLoading) {
        QMessageBox::information(this, "Info", "Model is loading, please wait...");
        return;
    }

    if (const QString file = QFileDialog::getSaveFileName(this, "Select Output MIDI File", "", "MIDI Files (*.mid)");
        !file.isEmpty()) {
        m_outputMidiLineEdit->setText(file);
        m_settings->setValue("MainWidget/outMidiPath", file);
    }
}

void MainWidget::onWavPathChanged(const QString &wavPath) const {
    if (!wavPath.isEmpty() && m_outputMidiLineEdit->text().isEmpty()) {
        generateMidiOutputPath(wavPath);
    }
}

void MainWidget::generateMidiOutputPath(const QString &wavPath) const {
    const QFileInfo fileInfo(wavPath);
    const QString baseName = fileInfo.baseName();
    const QString dir = fileInfo.absolutePath();
    const QString midiPath = dir + "/" + baseName + ".mid";
    m_outputMidiLineEdit->setText(midiPath);
    m_settings->setValue("MainWidget/outMidiPath", midiPath);
}

void MainWidget::onExportMidiTask() {
    if (m_isLoading) {
        QMessageBox::information(this, "Info", "Model is loading, please wait...");
        return;
    }

    if (!m_game) {
        QMessageBox::warning(this, "Warning", "Please load model first");
        return;
    }

    // Update parameters before running
    updateParameterValues();

    if (m_wavPathLineEdit->text().isEmpty() || m_outputMidiLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please fill input audio file and output MIDI file paths");
        return;
    }

    m_runButton->setEnabled(false);
    m_progressBar->setVisible(true);

    QFuture<void> future = QtConcurrent::run([this] {
        std::vector<Game::GameMidi> midis;
        std::string msg;
        m_progressBar->setValue(0);

        const bool success = m_game->get_midi(
            m_wavPathLineEdit->text().toLocal8Bit().toStdString(), midis, static_cast<float>(m_tempoSpin->value()), msg,
            [this](const int progress) {
                QMetaObject::invokeMethod(m_progressBar, "setValue", Qt::QueuedConnection, Q_ARG(int, progress));
            });

        if (success) {
            makeMidiFile(m_outputMidiLineEdit->text().toLocal8Bit().toStdString(), midis,
                         static_cast<float>(m_tempoSpin->value()));
            QMetaObject::invokeMethod(
                this, [this] { QMessageBox::information(this, "Success", "MIDI file generated!"); },
                Qt::QueuedConnection);
        } else {
            std::cerr << "Error: " << msg << std::endl;
            QMetaObject::invokeMethod(
                this,
                [this, msg] {
                    QMessageBox::critical(this, "Error", QString("Conversion failed: %1").arg(msg.c_str()));
                },
                Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(m_runButton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(m_progressBar, "setVisible", Qt::QueuedConnection, Q_ARG(bool, false));
    });
}