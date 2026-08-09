#include "MainWidget.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QSettings>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QUrl>
#include <fstream>
#include <iostream>

#include <wolf-midi/MidiFile.h>

#include "utils/DmlGpuUtils.h"

static QString replaceFileExtension(const QString &filePath, const QString &newExt);

namespace
{
    enum QueueColumn {
        QueueNumberColumn = 0,
        QueueStatusColumn,
        QueueInputFileColumn,
        QueueInputPathColumn,
        QueueOutputFileColumn,
        QueueOutputPathColumn,
        QueueLanguageColumn,
        QueueTempoColumn,
        QueueDetailsColumn,
        QueueColumnCount,
    };

    QTableWidgetItem *createQueueItem(const QString &text, const bool editable) {
        auto *item = new QTableWidgetItem(text);
        if (!editable) {
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
        return item;
    }

    QString combinePath(const QString &directory, const QString &fileName) {
        if (directory.trimmed().isEmpty() || fileName.trimmed().isEmpty()) {
            return {};
        }
        return QDir::cleanPath(QDir(directory.trimmed()).filePath(fileName.trimmed()));
    }
}

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
    : QWidget(parent), m_settings(settings), m_timeStepSeconds(0.01), m_framesPerSecond(1.0 / 0.01) {
    m_game = std::make_shared<Game::Game>();
    m_queueController = new InferenceQueueController(this);

    auto *mainLayout = new QVBoxLayout(this);

    setupModelGroup();
    setupProcessingGroup();
    setupActionButtons();
    setupQueueGroup();
    setupExecutionBar();
    setAcceptDrops(true);

    connect(m_queueController, &InferenceQueueController::jobsChanged, this, &MainWidget::refreshQueueTable);
    connect(m_queueController, &InferenceQueueController::runningChanged, this, &MainWidget::onQueueRunningChanged);
    connect(m_queueController, &InferenceQueueController::currentJobChanged, this, [this](const quint64 id) {
        m_currentQueueJobId = id;
        if (id == 0) {
            m_currentQueueJobLabel->setText(tr("Current: —"));
            return;
        }
        m_progressBar->setValue(0);
        for (const auto &job : m_queueController->jobs()) {
            if (job.id == id) {
                m_currentQueueJobLabel->setText(tr("Current: %1").arg(QFileInfo(job.inputPath).fileName()));
                break;
            }
        }
    });
    connect(m_queueController, &InferenceQueueController::queueFinished, this, [this](const bool stopped) {
        QMessageBox::information(this, tr("Queue"),
                                 stopped ? tr("The queue stopped after the current task.")
                                         : tr("The queue has finished."));
    });

    setModelLoadingStatus(ModelStatus::NotLoaded);

    max_audio_seg_length = m_settings->value("MainWidget/max_audio_seg_length", 60).toInt();
    m_settings->setValue("MainWidget/max_audio_seg_length", max_audio_seg_length);

    // Automatically load config when widget is initialized
    const auto modelPath = std::filesystem::path(m_modelPathEdit->text().toLocal8Bit().toStdString());
    if (!modelPath.empty()) {
        loadLanguagesFromConfig(modelPath);
        updateTimeStepInfo(modelPath);
    }

    retranslateUi();
}

MainWidget::~MainWidget() {
    delete m_queueController;
    m_queueController = nullptr;
}

void MainWidget::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void MainWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (!m_queueController->isRunning() && event->mimeData()->hasUrls() &&
        isQueueDropPosition(event->position())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void MainWidget::dragMoveEvent(QDragMoveEvent *event) {
    if (!m_queueController->isRunning() && event->mimeData()->hasUrls() &&
        isQueueDropPosition(event->position())) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void MainWidget::dropEvent(QDropEvent *event) {
    if (m_queueController->isRunning() || !event->mimeData()->hasUrls() ||
        !isQueueDropPosition(event->position())) {
        event->ignore();
        return;
    }

    QStringList supportedFiles;
    QStringList rejectedItems;
    const QSet<QString> supportedSuffixes{"wav", "flac", "mp3"};
    for (const QUrl &url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) {
            rejectedItems.push_back(url.toDisplayString());
            continue;
        }

        const QString localPath = QDir::toNativeSeparators(url.toLocalFile());
        const QFileInfo fileInfo(localPath);
        if (fileInfo.isFile() && supportedSuffixes.contains(fileInfo.suffix().toLower())) {
            supportedFiles.push_back(fileInfo.absoluteFilePath());
        } else {
            rejectedItems.push_back(localPath);
        }
    }

    event->acceptProposedAction();

    if (supportedFiles.isEmpty()) {
        QMessageBox::warning(
            this, tr("Warning"), tr("No supported audio files were dropped. Supported formats: WAV, FLAC, MP3."));
        return;
    }

    if (!rejectedItems.isEmpty()) {
        QMessageBox::warning(
            this, tr("Warning"),
            tr("Some dropped items were not added because they are folders or unsupported files:\n%1")
                .arg(rejectedItems.join('\n')));
    }

    addBatchJobsFromFiles(supportedFiles);
}

void MainWidget::retranslateUi() {
    if (m_modelGroup == nullptr) {
        return;
    }

    m_modelGroup->setTitle(tr("Model configuration"));
    m_modelPathLabel->setText(tr("Model path:"));
    m_browseModelBtn->setText(tr("Browse..."));
    m_providerLabel->setText(tr("Execution provider:"));
    for (int index = 0; index < m_providerCombo->count(); ++index) {
        const auto provider = static_cast<Game::ExecutionProvider>(m_providerCombo->itemData(index).toInt());
        m_providerCombo->setItemText(index, provider == Game::ExecutionProvider::CPU ? tr("CPU") : tr("DirectML"));
    }
    m_deviceLabel->setText(tr("Execution device:"));
    for (int index = 0; index < m_deviceCombo->count(); ++index) {
        if (m_deviceCombo->itemData(index).toInt() == -1) {
            m_deviceCombo->setItemText(index, tr("Default"));
        }
    }

    m_processingGroup->setTitle(tr("Processing parameters"));
    m_segThresholdLabel->setText(tr("Segmentation threshold (--seg-threshold):"));
    m_segRadiusLabel->setText(tr("Segmentation radius (frames, ms):"));
    m_estThresholdLabel->setText(tr("Estimation threshold (--est-threshold):"));
    m_segD3PMNStepsLabel->setText(tr("Sampling steps (--seg-d3pm-nsteps):"));
    const double ms = m_segRadiusFrameSpin->value() * (m_timeStepSeconds * 1000.0);
    m_segRadiusMsLabel->setText(tr("(%1 ms)").arg(ms, 0, 'f', 2));

    m_resetParamsBtn->setText(tr("Restore defaults"));

    m_toggleQueueBtn->setText(m_toggleQueueBtn->isChecked() ? tr("▼ Task queue (%1)").arg(m_queueController->jobs().size())
                                                             : tr("▶ Task queue (%1)").arg(m_queueController->jobs().size()));
    const QString queueDropTip = tr("Drop WAV, FLAC, or MP3 files here to add them to the queue.");
    m_toggleQueueBtn->setToolTip(queueDropTip);
    m_queueGroup->setTitle(tr("Task queue"));
    m_queueGroup->setToolTip(queueDropTip);
    m_queueTable->setToolTip(queueDropTip);
    m_batchAddButton->setText(tr("Add files..."));
    m_removeQueueButton->setText(tr("Remove"));
    m_moveQueueUpButton->setText(tr("Move up"));
    m_moveQueueDownButton->setText(tr("Move down"));
    m_retryFailedButton->setText(tr("Retry failed"));
    m_queueDefaultsLabel->setText(tr("New task defaults / batch settings:"));
    m_languageLabel->setText(tr("Language:"));
    m_tempoLabel->setText(tr("Tempo:"));
    m_applyQueueDefaultsButton->setText(tr("Apply to all editable tasks"));
    updateLanguageCombo();
    m_queueTable->setHorizontalHeaderLabels(
        {tr("#"), tr("Status"), tr("Input file"), tr("Input path"), tr("Output file"), tr("Output path"),
         tr("Language"), tr("Tempo (BPM)"), tr("Details")});
    m_startQueueButton->setText(tr("Start conversion"));
    if (m_queueController->isRunning() && !m_stopQueueButton->isEnabled()) {
        m_stopQueueButton->setText(tr("Stopping after current..."));
    } else {
        m_stopQueueButton->setText(tr("Stop after current"));
    }

    if (m_currentQueueJobId == 0) {
        m_currentQueueJobLabel->setText(tr("Current: —"));
    } else {
        for (const auto &job : m_queueController->jobs()) {
            if (job.id == m_currentQueueJobId) {
                m_currentQueueJobLabel->setText(tr("Current: %1").arg(QFileInfo(job.inputPath).fileName()));
                break;
            }
        }
    }
    setModelLoadingStatus(m_modelStatus);
    refreshQueueTable();
}

void MainWidget::setupModelGroup() {
    m_modelGroup = new QGroupBox(tr("Model configuration"));
    auto *layout = new QGridLayout(m_modelGroup);

    // Model path
    m_modelPathLabel = new QLabel(tr("Model path:"));
    layout->addWidget(m_modelPathLabel, 0, 0);
    m_modelPathEdit = new QLineEdit();

    const QString savedModelPath = m_settings->value("MainWidget/modelPath", "").toString();
    if (savedModelPath.isEmpty()) {
        m_modelPathEdit->setText(QApplication::applicationDirPath() + "/model/GAME-1.0.3-small-onnx");
    } else {
        m_modelPathEdit->setText(savedModelPath);
    }
    layout->addWidget(m_modelPathEdit, 0, 1, 1, 3);

    m_browseModelBtn = new QPushButton(tr("Browse..."));
    connect(m_browseModelBtn, &QPushButton::clicked, this, &MainWidget::browseModelPath);
    layout->addWidget(m_browseModelBtn, 0, 4);

    // Provider selection
    m_providerLabel = new QLabel(tr("Execution provider:"));
    layout->addWidget(m_providerLabel, 1, 0);
    m_providerCombo = new QComboBox();
    m_providerCombo->addItem(tr("CPU"), static_cast<int>(Game::ExecutionProvider::CPU));
#ifdef _WIN32
    m_providerCombo->addItem(tr("DirectML"), static_cast<int>(Game::ExecutionProvider::DML));
#endif
    layout->addWidget(m_providerCombo, 1, 1);

    // Device selection
    m_deviceLabel = new QLabel(tr("Execution device:"));
    layout->addWidget(m_deviceLabel, 1, 2);
    m_deviceCombo = new QComboBox();
    m_deviceCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_deviceCombo->addItem(tr("Default"), -1);
    layout->addWidget(m_deviceCombo, 1, 3);

    // Refresh devices when provider changes
    connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        updateDeviceList();
        setModelLoadingStatus(ModelStatus::ConfigurationChanged);
    });

    // Status label
    m_modelStatusLabel = new QLabel(tr("Not loaded"));
    m_modelStatusLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
    layout->addWidget(m_modelStatusLabel, 2, 0, 1, 3);

    connect(m_modelPathEdit, &QLineEdit::textChanged, this,
            [this] { setModelLoadingStatus(ModelStatus::ConfigurationChanged); });
    connect(m_deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this] { setModelLoadingStatus(ModelStatus::ConfigurationChanged); });

    // Removed Load Model button

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(m_modelGroup);
}

void MainWidget::setModelLoadingStatus(const ModelStatus status) {
    QMetaObject::invokeMethod(
        this,
        [this, status] {
            m_modelStatus = status;
            QString text;
            switch (status) {
            case ModelStatus::NotLoaded:
                text = tr("Not loaded");
                break;
            case ModelStatus::ConfigurationChanged:
                text = tr("Model settings changed; the model will reload on the next run.");
                break;
            case ModelStatus::PathMissing:
                text = tr("Select a model path.");
                break;
            case ModelStatus::Loading:
                text = tr("Loading model; this may take 3–10 seconds...");
                break;
            case ModelStatus::Loaded:
                text = tr("Model loaded successfully.");
                break;
            case ModelStatus::LoadFailed:
                text = tr("Failed to load the model.");
                break;
            }

            if (status == ModelStatus::Loaded) {
                m_modelStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
            } else if (status == ModelStatus::LoadFailed || status == ModelStatus::PathMissing) {
                m_modelStatusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
            } else if (status == ModelStatus::Loading) {
                m_modelStatusLabel->setStyleSheet("QLabel { color: blue; font-weight: bold; }");
            } else {
                m_modelStatusLabel->setStyleSheet("QLabel { color: gray; font-style: italic; }");
            }

            m_modelStatusLabel->setText(text);
        },
        Qt::QueuedConnection);
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
    m_deviceCombo->addItem(tr("Default"), -1);
}

void MainWidget::setupProcessingGroup() {
    m_processingGroup = new QGroupBox(tr("Processing parameters"));
    auto *layout = new QGridLayout(m_processingGroup);

    // Row 0: Segmentation threshold
    m_segThresholdLabel = new QLabel(tr("Segmentation threshold (--seg-threshold):"));
    layout->addWidget(m_segThresholdLabel, 0, 0);
    m_segThresholdSpin = new QDoubleSpinBox();
    m_segThresholdSpin->setRange(0.0, 0.99);
    m_segThresholdSpin->setSingleStep(0.01);
    m_segThresholdSpin->setValue(0.2);
    layout->addWidget(m_segThresholdSpin, 0, 1);
    m_segThresholdSpin->setValue(m_settings->value("MainWidget/segThreshold", 0.2).toFloat());

    connect(m_segThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](const double value) { m_settings->setValue("MainWidget/segThreshold", value); });

    // Segmentation radius in frames
    m_segRadiusLabel = new QLabel(tr("Segmentation radius (frames, ms):"));
    layout->addWidget(m_segRadiusLabel, 0, 2);
    m_segRadiusFrameSpin = new QSpinBox();
    m_segRadiusFrameSpin->setRange(1, 1000);
    m_segRadiusFrameSpin->setSingleStep(1);
    m_segRadiusFrameSpin->setValue(2);
    layout->addWidget(m_segRadiusFrameSpin, 0, 3);
    m_segRadiusFrameSpin->setValue(m_settings->value("MainWidget/segRadiusFrame", 2).toInt());

    connect(m_segRadiusFrameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](const int value) { m_settings->setValue("MainWidget/segRadiusFrame", value); });

    m_segRadiusMsLabel = new QLabel(tr("(ms)"));
    layout->addWidget(m_segRadiusMsLabel, 0, 4);

    // Connect frame spin to update ms label
    connect(m_segRadiusFrameSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](const int value) {
        const double ms = value * (m_timeStepSeconds * 1000.0);
        m_segRadiusMsLabel->setText(tr("(%1 ms)").arg(ms, 0, 'f', 2));
    });

    // Row 1: Estimation threshold
    m_estThresholdLabel = new QLabel(tr("Estimation threshold (--est-threshold):"));
    layout->addWidget(m_estThresholdLabel, 1, 0);
    m_estThresholdSpin = new QDoubleSpinBox();
    m_estThresholdSpin->setRange(0.0, 0.99);
    m_estThresholdSpin->setSingleStep(0.01);
    m_estThresholdSpin->setValue(0.2);
    layout->addWidget(m_estThresholdSpin, 1, 1);
    m_estThresholdSpin->setValue(m_settings->value("MainWidget/estThreshold", 0.2).toFloat());

    connect(m_estThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](const double value) { m_settings->setValue("MainWidget/estThreshold", value); });

    // D3PM nsteps
    m_segD3PMNStepsLabel = new QLabel(tr("Sampling steps (--seg-d3pm-nsteps):"));
    layout->addWidget(m_segD3PMNStepsLabel, 1, 2);
    m_segD3PMNStepsCombo = new QComboBox();
    m_segD3PMNStepsCombo->addItem("1", 1);
    m_segD3PMNStepsCombo->addItem("2", 2);
    m_segD3PMNStepsCombo->addItem("4", 4);
    m_segD3PMNStepsCombo->addItem("8", 8);
    m_segD3PMNStepsCombo->addItem("16", 16);
    m_segD3PMNStepsCombo->setCurrentIndex(3);
    layout->addWidget(m_segD3PMNStepsCombo, 1, 3);

    m_segD3PMNStepsCombo->setCurrentIndex(m_settings->value("MainWidget/segD3PMNSteps", 3).toInt());

    // Connect D3PM parameter to update immediately
    connect(m_segD3PMNStepsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](const int value) { m_settings->setValue("MainWidget/segD3PMNSteps", value); });

    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(m_processingGroup);
}

void MainWidget::setupActionButtons() {
    auto *layout = new QHBoxLayout();

    m_resetParamsBtn = new QPushButton(tr("Restore defaults"));
    connect(m_resetParamsBtn, &QPushButton::clicked, this, &MainWidget::resetToDefaults);
    layout->addWidget(m_resetParamsBtn);

    layout->addStretch();

    m_toggleQueueBtn = new QPushButton(tr("▶ Task queue (0)"));
    m_toggleQueueBtn->setCheckable(true);
    connect(m_toggleQueueBtn, &QPushButton::toggled, this, [this](const bool expanded) {
        m_queueGroup->setVisible(expanded);
        m_toggleQueueBtn->setText(expanded ? tr("▼ Task queue (%1)").arg(m_queueController->jobs().size())
                                           : tr("▶ Task queue (%1)").arg(m_queueController->jobs().size()));
    });
    layout->addWidget(m_toggleQueueBtn);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addLayout(layout);
}

void MainWidget::setupQueueGroup() {
    m_queueGroup = new QGroupBox(tr("Task queue"));
    auto *layout = new QVBoxLayout(m_queueGroup);

    auto *toolbar = new QHBoxLayout();
    m_batchAddButton = new QPushButton(tr("Add files..."));
    m_removeQueueButton = new QPushButton(tr("Remove"));
    m_moveQueueUpButton = new QPushButton(tr("Move up"));
    m_moveQueueDownButton = new QPushButton(tr("Move down"));
    m_retryFailedButton = new QPushButton(tr("Retry failed"));
    toolbar->addWidget(m_batchAddButton);
    toolbar->addWidget(m_removeQueueButton);
    toolbar->addWidget(m_moveQueueUpButton);
    toolbar->addWidget(m_moveQueueDownButton);
    toolbar->addWidget(m_retryFailedButton);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto *defaultsLayout = new QHBoxLayout();
    m_queueDefaultsLabel = new QLabel(tr("New task defaults / batch settings:"));
    m_languageLabel = new QLabel(tr("Language:"));
    m_languageCombo = new QComboBox();
    m_languageCombo->setObjectName("queueDefaultLanguageCombo");
    m_languageCombo->addItem(tr("Default"), 0);
    m_tempoLabel = new QLabel(tr("Tempo:"));
    m_tempoSpin = new QDoubleSpinBox();
    m_tempoSpin->setObjectName("queueDefaultTempoSpin");
    m_tempoSpin->setRange(1.0, 300.0);
    m_tempoSpin->setSingleStep(1.0);
    m_tempoSpin->setDecimals(0);
    m_tempoSpin->setValue(m_settings->value("MainWidget/tempo", 120.0).toDouble());
    m_applyQueueDefaultsButton = new QPushButton(tr("Apply to all editable tasks"));
    m_applyQueueDefaultsButton->setObjectName("applyQueueDefaultsButton");
    defaultsLayout->addWidget(m_queueDefaultsLabel);
    defaultsLayout->addWidget(m_languageLabel);
    defaultsLayout->addWidget(m_languageCombo);
    defaultsLayout->addWidget(m_tempoLabel);
    defaultsLayout->addWidget(m_tempoSpin);
    defaultsLayout->addStretch();
    defaultsLayout->addWidget(m_applyQueueDefaultsButton);
    layout->addLayout(defaultsLayout);

    connect(m_languageCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](const int) {
        m_settings->setValue("MainWidget/languageId", m_languageCombo->currentData().toInt());
    });
    connect(m_tempoSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](const double value) { m_settings->setValue("MainWidget/tempo", value); });
    connect(m_applyQueueDefaultsButton, &QPushButton::clicked,
            this, &MainWidget::applyQueueDefaultsToEditableJobs);

    m_queueTable = new QTableWidget(0, QueueColumnCount, m_queueGroup);
    m_queueTable->setObjectName("queueTable");
    m_queueTable->setHorizontalHeaderLabels(
        {tr("#"), tr("Status"), tr("Input file"), tr("Input path"), tr("Output file"), tr("Output path"),
         tr("Language"), tr("Tempo (BPM)"), tr("Details")});
    m_queueTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_queueTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_queueTable->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed |
                                  QAbstractItemView::SelectedClicked);
    m_queueTable->verticalHeader()->setVisible(false);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueNumberColumn, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueStatusColumn, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueInputFileColumn, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueInputPathColumn, QHeaderView::Stretch);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueOutputFileColumn, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueOutputPathColumn, QHeaderView::Stretch);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueLanguageColumn, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueTempoColumn, QHeaderView::ResizeToContents);
    m_queueTable->horizontalHeader()->setSectionResizeMode(QueueDetailsColumn, QHeaderView::Stretch);
    layout->addWidget(m_queueTable);

    connect(m_batchAddButton, &QPushButton::clicked, this, &MainWidget::addBatchJobs);
    connect(m_removeQueueButton, &QPushButton::clicked, this, &MainWidget::removeSelectedQueueJob);
    connect(m_moveQueueUpButton, &QPushButton::clicked, this, &MainWidget::moveSelectedQueueJobUp);
    connect(m_moveQueueDownButton, &QPushButton::clicked, this, &MainWidget::moveSelectedQueueJobDown);
    connect(m_retryFailedButton, &QPushButton::clicked, m_queueController,
            &InferenceQueueController::resetFailedJobs);
    connect(m_queueTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        const int row = item->row();
        QTimer::singleShot(0, this, [this, row] { updateQueueJobFromRow(row); });
    });

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addWidget(m_queueGroup, 1);
    m_toggleQueueBtn->setChecked(true);
}

void MainWidget::setupExecutionBar() {
    auto *executionLayout = new QVBoxLayout();
    auto *statusLayout = new QHBoxLayout();
    m_currentQueueJobLabel = new QLabel(tr("Current: —"));
    m_queueSummaryLabel = new QLabel(tr("Total: 0"));
    statusLayout->addWidget(m_currentQueueJobLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_queueSummaryLabel);
    executionLayout->addLayout(statusLayout);

    auto *controlsLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar();
    m_progressBar->setObjectName("conversionProgressBar");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_startQueueButton = new QPushButton(tr("Start conversion"));
    m_stopQueueButton = new QPushButton(tr("Stop after current"));
    m_stopQueueButton->setEnabled(false);
    controlsLayout->addWidget(m_progressBar, 1);
    controlsLayout->addWidget(m_startQueueButton);
    controlsLayout->addWidget(m_stopQueueButton);
    executionLayout->addLayout(controlsLayout);

    connect(m_startQueueButton, &QPushButton::clicked, this, &MainWidget::startQueue);
    connect(m_stopQueueButton, &QPushButton::clicked, this, &MainWidget::stopQueueAfterCurrent);

    auto *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
    mainLayout->addLayout(executionLayout);
}

QVector<QPair<int, QString>> MainWidget::availableLanguages() const {
    QVector<QPair<int, QString>> languages;
    languages.reserve(static_cast<qsizetype>(m_languageIdToName.size()));
    for (const auto &[id, name] : m_languageIdToName) {
        languages.push_back({id, id == 0 ? tr("Default") : QString::fromStdString(name)});
    }
    return languages;
}

void MainWidget::addBatchJobs() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Select input audio files"), {},
        tr("Audio files (*.wav *.flac *.mp3);;WAV files (*.wav);;FLAC files (*.flac);;MP3 files (*.mp3)"));
    addBatchJobsFromFiles(files);
}

bool MainWidget::isQueueDropPosition(const QPointF &position) const {
    const QPoint widgetPosition = position.toPoint();
    const auto containsPosition = [this, widgetPosition](const QWidget *target) {
        return target != nullptr && target->isVisible() &&
            target->rect().contains(target->mapFrom(this, widgetPosition));
    };
    return containsPosition(m_toggleQueueBtn) || containsPosition(m_queueGroup);
}

void MainWidget::addBatchJobsFromFiles(const QStringList &files) {
    if (files.isEmpty()) {
        return;
    }

    for (const auto &file : files) {
        QueueJob job;
        job.inputPath = file;
        job.outputPath = replaceFileExtension(file, "mid");
        job.languageId = m_languageCombo->currentData().toInt();
        job.languageName = m_languageCombo->currentText();
        job.tempo = m_tempoSpin->value();
        m_queueController->addJob(std::move(job));
    }
    m_toggleQueueBtn->setChecked(true);
}

void MainWidget::applyQueueDefaultsToEditableJobs() {
    m_queueController->applyDefaultsToEditableJobs(
        m_languageCombo->currentData().toInt(), m_languageCombo->currentText(), m_tempoSpin->value());
}

quint64 MainWidget::selectedQueueJobId() const {
    const int row = m_queueTable->currentRow();
    if (row < 0) {
        return 0;
    }
    const auto *item = m_queueTable->item(row, 0);
    return item ? item->data(Qt::UserRole).toULongLong() : 0;
}

void MainWidget::updateQueueJobFromRow(const int row) {
    if (m_refreshingQueueTable || m_queueController->isRunning() || row < 0 || row >= m_queueTable->rowCount()) {
        return;
    }

    const auto *numberItem = m_queueTable->item(row, QueueNumberColumn);
    const quint64 id = numberItem == nullptr ? 0 : numberItem->data(Qt::UserRole).toULongLong();
    if (id == 0) {
        return;
    }

    for (const auto &job : m_queueController->jobs()) {
        if (job.id != id) {
            continue;
        }
        if (job.status == QueueJobStatus::Completed) {
            return;
        }

        const auto *inputFileItem = m_queueTable->item(row, QueueInputFileColumn);
        const auto *inputPathItem = m_queueTable->item(row, QueueInputPathColumn);
        const auto *outputFileItem = m_queueTable->item(row, QueueOutputFileColumn);
        const auto *outputPathItem = m_queueTable->item(row, QueueOutputPathColumn);
        if (inputFileItem == nullptr || inputPathItem == nullptr || outputFileItem == nullptr ||
            outputPathItem == nullptr) {
            return;
        }

        QueueJob updated = job;
        updated.inputPath = combinePath(inputPathItem->text(), inputFileItem->text());
        updated.outputPath = combinePath(outputPathItem->text(), outputFileItem->text());
        if (updated.inputPath.isEmpty() || updated.outputPath.isEmpty()) {
            refreshQueueTable();
            return;
        }
        m_queueController->updateJob(id, updated);
        return;
    }
}

void MainWidget::removeSelectedQueueJob() {
    const quint64 id = selectedQueueJobId();
    if (id != 0) {
        m_queueController->removeJob(id);
    }
}

void MainWidget::moveSelectedQueueJobUp() {
    const quint64 id = selectedQueueJobId();
    if (id != 0) {
        m_queueController->moveJob(id, -1);
    }
}

void MainWidget::moveSelectedQueueJobDown() {
    const quint64 id = selectedQueueJobId();
    if (id != 0) {
        m_queueController->moveJob(id, 1);
    }
}

void MainWidget::refreshQueueTable() {
    const quint64 selectedId = selectedQueueJobId();
    const auto &jobs = m_queueController->jobs();
    const QSignalBlocker tableBlocker(m_queueTable);
    m_refreshingQueueTable = true;
    m_queueTable->setRowCount(jobs.size());

    int completed = 0;
    int failed = 0;
    int running = 0;
    int editable = 0;
    for (int row = 0; row < jobs.size(); ++row) {
        const auto &job = jobs[row];
        QString status;
        switch (job.status) {
        case QueueJobStatus::Pending:
            status = tr("Pending");
            ++editable;
            break;
        case QueueJobStatus::Running:
            status = tr("Running");
            ++running;
            m_progressBar->setValue(job.progress);
            break;
        case QueueJobStatus::Completed:
            status = tr("Completed");
            ++completed;
            break;
        case QueueJobStatus::Failed:
            status = tr("Failed");
            ++failed;
            ++editable;
            break;
        }

        const bool editable = !m_queueController->isRunning() && job.status != QueueJobStatus::Completed;
        const QFileInfo inputInfo(job.inputPath);
        const QFileInfo outputInfo(job.outputPath);

        auto *numberItem = createQueueItem(QString::number(row + 1), false);
        numberItem->setData(Qt::UserRole, QVariant::fromValue(job.id));
        m_queueTable->setItem(row, QueueNumberColumn, numberItem);
        m_queueTable->setItem(row, QueueStatusColumn, createQueueItem(status, false));
        m_queueTable->setItem(row, QueueInputFileColumn, createQueueItem(inputInfo.fileName(), false));
        m_queueTable->setItem(
            row, QueueInputPathColumn,
            createQueueItem(QDir::toNativeSeparators(inputInfo.absolutePath()), editable));
        m_queueTable->setItem(row, QueueOutputFileColumn, createQueueItem(outputInfo.fileName(), editable));
        m_queueTable->setItem(
            row, QueueOutputPathColumn,
            createQueueItem(QDir::toNativeSeparators(outputInfo.absolutePath()), editable));

        auto *languageCombo = new QComboBox(m_queueTable);
        for (const auto &[languageId, languageName] : availableLanguages()) {
            languageCombo->addItem(languageName, languageId);
        }
        const int languageIndex = languageCombo->findData(job.languageId);
        languageCombo->setCurrentIndex(languageIndex >= 0 ? languageIndex : 0);
        languageCombo->setEnabled(editable);
        connect(languageCombo, QOverload<int>::of(&QComboBox::activated), this,
                [this, id = job.id, languageCombo](const int index) {
                    const int languageId = languageCombo->itemData(index).toInt();
                    const QString languageName = languageCombo->itemText(index);
                    QTimer::singleShot(0, this, [this, id, languageId, languageName] {
                        if (m_queueController->isRunning()) {
                            return;
                        }
                        for (const auto &current : m_queueController->jobs()) {
                            if (current.id == id && current.status != QueueJobStatus::Completed) {
                                QueueJob updated = current;
                                updated.languageId = languageId;
                                updated.languageName = languageName;
                                m_queueController->updateJob(id, updated);
                                return;
                            }
                        }
                    });
                });
        m_queueTable->setCellWidget(row, QueueLanguageColumn, languageCombo);

        auto *tempoSpin = new QDoubleSpinBox(m_queueTable);
        tempoSpin->setRange(1.0, 300.0);
        tempoSpin->setSingleStep(1.0);
        tempoSpin->setDecimals(0);
        tempoSpin->setValue(job.tempo);
        tempoSpin->setEnabled(editable);
        connect(tempoSpin, &QDoubleSpinBox::editingFinished, this, [this, id = job.id, tempoSpin] {
            const double tempo = tempoSpin->value();
            QTimer::singleShot(0, this, [this, id, tempo] {
                if (m_queueController->isRunning()) {
                    return;
                }
                for (const auto &current : m_queueController->jobs()) {
                    if (current.id == id && current.status != QueueJobStatus::Completed) {
                        QueueJob updated = current;
                        updated.tempo = tempo;
                        m_queueController->updateJob(id, updated);
                        return;
                    }
                }
            });
        });
        m_queueTable->setCellWidget(row, QueueTempoColumn, tempoSpin);

        const QString details = job.status == QueueJobStatus::Running ? tr("%1%").arg(job.progress) : job.error;
        auto *detailsItem = createQueueItem(details, false);
        detailsItem->setToolTip(details);
        m_queueTable->setItem(row, QueueDetailsColumn, detailsItem);

        if (job.id == selectedId) {
            m_queueTable->selectRow(row);
        }
    }
    m_refreshingQueueTable = false;

    m_queueSummaryLabel->setText(
        tr("Total: %1 · Completed: %2 · Failed: %3").arg(jobs.size()).arg(completed).arg(failed));
    m_toggleQueueBtn->setText(m_toggleQueueBtn->isChecked() ? tr("▼ Task queue (%1)").arg(jobs.size())
                                                            : tr("▶ Task queue (%1)").arg(jobs.size()));
    m_startQueueButton->setEnabled(!m_queueController->isRunning() && !jobs.isEmpty());
    m_retryFailedButton->setEnabled(!m_queueController->isRunning() && failed > 0);
    m_applyQueueDefaultsButton->setEnabled(!m_queueController->isRunning() && editable > 0);
    if (running == 0 && !m_queueController->isRunning()) {
        m_progressBar->setValue(0);
    }
}

bool MainWidget::validateQueueBeforeStart() {
    bool hasPending = false;
    QSet<QString> outputPaths;
    QStringList existingOutputs;

    for (const auto &job : m_queueController->jobs()) {
        if (job.status != QueueJobStatus::Pending) {
            continue;
        }
        hasPending = true;
        if (!QFileInfo(job.inputPath).isFile()) {
            QMessageBox::critical(this, tr("Queue validation"),
                                  tr("The input audio file does not exist: %1").arg(job.inputPath));
            return false;
        }
        const QFileInfo outputInfo(job.outputPath);
        if (job.outputPath.trimmed().isEmpty() || outputInfo.isDir() || !QDir(outputInfo.absolutePath()).exists()) {
            QMessageBox::critical(this, tr("Queue validation"),
                                  tr("Invalid output MIDI path: %1").arg(job.outputPath));
            return false;
        }

        QString normalizedOutput = QDir::cleanPath(QFileInfo(job.outputPath).absoluteFilePath());
#ifdef Q_OS_WIN
        normalizedOutput = normalizedOutput.toLower();
#endif
        if (outputPaths.contains(normalizedOutput)) {
            QMessageBox::critical(this, tr("Queue validation"),
                                  tr("Multiple tasks use the same output MIDI path: %1").arg(job.outputPath));
            return false;
        }
        outputPaths.insert(normalizedOutput);
        if (QFileInfo::exists(job.outputPath)) {
            existingOutputs.push_back(job.outputPath);
        }
    }

    if (!hasPending) {
        QMessageBox::information(this, tr("Queue"), tr("There are no pending tasks to run."));
        return false;
    }
    if (!existingOutputs.isEmpty()) {
        const QString displayed = existingOutputs.mid(0, 20).join("\n");
        const QString suffix = existingOutputs.size() > 20 ? tr("\n...and %1 more").arg(existingOutputs.size() - 20)
                                                           : QString();
        const auto answer = QMessageBox::question(
            this, tr("Confirm overwrite"),
            tr("The following output files already exist:\n%1%2\n\nOverwrite them?").arg(displayed, suffix),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            return false;
        }
    }
    return true;
}

void MainWidget::startQueue() {
    if (!validateQueueBeforeStart()) {
        return;
    }

    const ModelSelection model = currentModelSelection();
    const ProcessingParameters sharedParameters = currentProcessingParameters();
    struct QueueRuntimeState {
        bool modelAttempted = false;
        bool modelReady = false;
        QString modelError;
    };
    const auto runtime = std::make_shared<QueueRuntimeState>();

    const bool started = m_queueController->start(
        [this, model, sharedParameters, runtime](const QueueJob &job,
                                                 const InferenceQueueController::ProgressCallback &progress,
                                                 QString &error) {
            if (!runtime->modelAttempted) {
                runtime->modelAttempted = true;
                const bool modelMatches = m_loadedModelSelection && m_loadedModelSelection->matches(model);
                if (!m_game->is_open() || !modelMatches) {
                    std::string modelMessage;
                    runtime->modelReady = loadModel(model, modelMessage);
                    runtime->modelError = QString::fromLocal8Bit(modelMessage);
                } else {
                    runtime->modelReady = true;
                }
            }
            if (!runtime->modelReady) {
                error = runtime->modelError.isEmpty() ? tr("Failed to load the model.") : runtime->modelError;
                return false;
            }

            if (!updateParameterValues(sharedParameters)) {
                error = tr("Failed to apply processing parameters.");
                return false;
            }
            m_game->set_language(job.languageId);
            const float tempo = static_cast<float>(job.tempo);

            std::vector<Game::GameMidi> midis;
            std::string message;
            const bool success = m_game->get_midi(
                std::filesystem::path(job.inputPath.toLocal8Bit().toStdString()), midis, tempo, message,
                progress, max_audio_seg_length);
            if (!success) {
                error = QString::fromLocal8Bit(message);
                return false;
            }

            makeMidiFile(std::filesystem::path(job.outputPath.toLocal8Bit().toStdString()), std::move(midis),
                         tempo);
            return true;
        });

    if (!started) {
        QMessageBox::information(this, tr("Queue"), tr("The queue could not be started."));
    }
}

void MainWidget::stopQueueAfterCurrent() {
    m_queueController->stopAfterCurrent();
    m_stopQueueButton->setEnabled(false);
    m_stopQueueButton->setText(tr("Stopping after current..."));
}

void MainWidget::onQueueRunningChanged(const bool running) {
    setAcceptDrops(!running);
    setControlsEnabled(!running);
    m_batchAddButton->setEnabled(!running);
    m_removeQueueButton->setEnabled(!running);
    m_moveQueueUpButton->setEnabled(!running);
    m_moveQueueDownButton->setEnabled(!running);
    m_retryFailedButton->setEnabled(!running);
    m_startQueueButton->setEnabled(!running && !m_queueController->jobs().isEmpty());
    m_stopQueueButton->setEnabled(running);
    m_stopQueueButton->setText(tr("Stop after current"));
    refreshQueueTable();
}

void MainWidget::browseModelPath() {
    const QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select model directory"), m_modelPathEdit->text(),
                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty()) {
        m_modelPathEdit->setText(dir);
        m_settings->setValue("MainWidget/modelPath", dir);

        // Auto-load config when model path changes
        loadLanguagesFromConfig(std::filesystem::path(dir.toStdWString()));
        updateTimeStepInfo(std::filesystem::path(dir.toStdWString()));
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
        m_timeStepSeconds = 0.01;
        m_framesPerSecond = 1.0 / 0.01;
    }

    // Update ms label display
    const int currentValue = m_segRadiusFrameSpin->value();
    const double ms = currentValue * (m_timeStepSeconds * 1000.0);
    m_segRadiusMsLabel->setText(tr("(%1 ms)").arg(ms, 0, 'f', 2));
}

MainWidget::ModelSelection MainWidget::currentModelSelection() const {
    return {
        std::filesystem::path(m_modelPathEdit->text().toLocal8Bit().toStdString()),
        static_cast<Game::ExecutionProvider>(m_providerCombo->currentData().toInt()),
        m_deviceCombo->currentData().toInt(),
    };
}

MainWidget::ProcessingParameters MainWidget::currentProcessingParameters() const {
    ProcessingParameters parameters;
    parameters.segThreshold = static_cast<float>(m_segThresholdSpin->value());
    parameters.segRadiusFrames = static_cast<float>(m_segRadiusFrameSpin->value());
    parameters.estThreshold = static_cast<float>(m_estThresholdSpin->value());

    const int nSteps = m_segD3PMNStepsCombo->currentData().toInt();
    if (nSteps > 0) {
        constexpr float t0 = 0.0f;
        const float step = (1.0f - t0) / static_cast<float>(nSteps);
        for (int i = 0; i < nSteps; ++i) {
            parameters.d3pmTs.push_back(t0 + static_cast<float>(i) * step);
        }
    }
    return parameters;
}

bool MainWidget::loadModel(const ModelSelection &selection, std::string &message) {
    if (selection.path.empty()) {
        setModelLoadingStatus(ModelStatus::PathMissing);
        message = tr("Select a model path.").toLocal8Bit().toStdString();
        return false;
    }

    std::string msg;

    QMetaObject::invokeMethod(
        this, [this] { setModelLoadingStatus(ModelStatus::Loading); }, Qt::QueuedConnection);
    if (m_game->load_model(selection.path, selection.provider, selection.deviceId, msg)) {
        m_loadedModelSelection = selection;
        const QString modelPathText = QString::fromUtf8(selection.path.u8string().c_str());
        // Successfully loaded, update UI
        QMetaObject::invokeMethod(
            this,
            [this, modelPathText] {
                setModelLoadingStatus(ModelStatus::Loaded);
                m_settings->setValue("MainWidget/modelPath", modelPathText);
            },
            Qt::QueuedConnection);
    } else {
        message = tr("Failed to load the model: %1").arg(QString::fromLocal8Bit(msg)).toLocal8Bit().toStdString();
        QMetaObject::invokeMethod(
            this, [this] { setModelLoadingStatus(ModelStatus::LoadFailed); }, Qt::QueuedConnection);
        return false;
    }
    return true;
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
    const int currentId = m_settings->value("MainWidget/languageId", m_languageCombo->currentData().toInt()).toInt();
    const QSignalBlocker blocker(m_languageCombo);

    // Clear and repopulate the combo box
    m_languageCombo->clear();

    // Add languages sorted by ID
    for (const auto &[id, name] : m_languageIdToName) {
        m_languageCombo->addItem(id == 0 ? tr("Default") : QString::fromStdString(name), id);
    }

    // Restore previous selection or default to 0
    const int index = m_languageCombo->findData(currentId);
    if (index != -1) {
        m_languageCombo->setCurrentIndex(index);
    } else {
        m_languageCombo->setCurrentIndex(0);
    }
    m_settings->setValue("MainWidget/languageId", m_languageCombo->currentData().toInt());
}

bool MainWidget::updateParameterValues(const ProcessingParameters &parameters) const {
    if (!m_game)
        return false;

    m_game->set_seg_threshold(parameters.segThreshold);
    m_game->set_seg_radius_frames(parameters.segRadiusFrames);
    m_game->set_est_threshold(parameters.estThreshold);
    m_game->set_d3pm_ts(parameters.d3pmTs);

    return true;
}

void MainWidget::setControlsEnabled(const bool enabled) const {
    m_modelPathEdit->setEnabled(enabled);
    m_browseModelBtn->setEnabled(enabled);
    m_providerCombo->setEnabled(enabled);
    m_deviceCombo->setEnabled(enabled);
    m_segThresholdSpin->setEnabled(enabled);
    m_segRadiusFrameSpin->setEnabled(enabled);
    m_estThresholdSpin->setEnabled(enabled);
    m_segD3PMNStepsCombo->setEnabled(enabled);
    m_languageCombo->setEnabled(enabled);
    m_tempoSpin->setEnabled(enabled);
    m_applyQueueDefaultsButton->setEnabled(enabled && !m_queueController->jobs().isEmpty());
    m_resetParamsBtn->setEnabled(enabled);
    m_batchAddButton->setEnabled(enabled);
    m_removeQueueButton->setEnabled(enabled);
    m_moveQueueUpButton->setEnabled(enabled);
    m_moveQueueDownButton->setEnabled(enabled);
    m_retryFailedButton->setEnabled(enabled);
    m_startQueueButton->setEnabled(enabled && !m_queueController->jobs().isEmpty());
    m_stopQueueButton->setEnabled(false);
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
    m_segRadiusMsLabel->setText(tr("(%1 ms)").arg(ms, 0, 'f', 2));
}

static QString replaceFileExtension(const QString &filePath, const QString &newExt) {
    const QFileInfo info(filePath);
    return info.absolutePath() + QDir::separator() + info.completeBaseName() + "." + newExt;
}
