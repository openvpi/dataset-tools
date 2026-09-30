#include "MainWindow.h"

#include <algorithm>
#include <filesystem>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QTextCursor>

#include "../util/AlignThread.h"
#include "../util/DmlGpuUtils.h"
#include "../util/PathUtils.h"

namespace fs = std::filesystem;

namespace Tifa {

    namespace {

        /** Languages of the built-in front end, in the order of the combo box. */
        constexpr Language kLanguages[] = {Language::Mandarin, Language::Cantonese, Language::Japanese,
                                           Language::English};

        /** The five graphs of an exported model. */
        constexpr const char *kGraphs[] = {"spectrogram", "model", "prepare", "score", "select"};

        QString defaultDirectory(const QString &name) {
#ifdef Q_OS_MAC
            return QDir::cleanPath(QApplication::applicationDirPath() + "/../Resources/" + name);
#else
            return QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + name);
#endif
        }

    } // namespace

    MainWindow::MainWindow(QWidget *parent) : AsyncTaskWindow(parent) {
        m_errorFormat.setForeground(Qt::red);
        setRunButtonText("Run Alignment");
        MainWindow::init();
        slot_loadModel();
    }

    MainWindow::~MainWindow() {
        delete m_aligner;
    }

    void MainWindow::init() {
        auto *modelLabel = new QLabel("TIFA model folder:", this);
        m_modelEdit = new QLineEdit(this);
        m_modelEdit->setText(defaultDirectory("model/Tifa"));
        auto *modelBrowseBtn = new QPushButton("Browse...", this);
        m_modelLoadBtn = new QPushButton("Load Model", this);
        m_modelStatusLabel = new QLabel("Model not loaded", this);
        auto *modelLayout = new QHBoxLayout();
        modelLayout->addWidget(modelLabel);
        modelLayout->addWidget(m_modelEdit);
        modelLayout->addWidget(modelBrowseBtn);
        modelLayout->addWidget(m_modelLoadBtn);
        modelLayout->addWidget(m_modelStatusLabel);
        connect(modelBrowseBtn, &QPushButton::clicked, this, &MainWindow::slot_browseModel);
        connect(m_modelLoadBtn, &QPushButton::clicked, this, &MainWindow::slot_loadModel);
        m_topLayout->addLayout(modelLayout);

        auto *dictLabel = new QLabel("Dictionary folder:", this);
        m_dictEdit = new QLineEdit(this);
        m_dictEdit->setText(defaultDirectory("dict"));
        auto *dictBtn = new QPushButton("Browse...", this);
        auto *dictLayout = new QHBoxLayout();
        dictLayout->addWidget(dictLabel);
        dictLayout->addWidget(m_dictEdit);
        dictLayout->addWidget(dictBtn);
        connect(dictBtn, &QPushButton::clicked, this, &MainWindow::slot_browseDict);
        m_rightPanel->addLayout(dictLayout);

        auto *outLabel = new QLabel("Output TextGrid directory:", this);
        m_outTgEdit = new QLineEdit(this);
        m_outTgEdit->setText("");
        auto *outBtn = new QPushButton("Browse...", this);
        auto *outLayout = new QHBoxLayout();
        outLayout->addWidget(outLabel);
        outLayout->addWidget(m_outTgEdit);
        outLayout->addWidget(outBtn);
        connect(outBtn, &QPushButton::clicked, this, &MainWindow::slot_outTgPath);
        m_rightPanel->addLayout(outLayout);

        auto *languageLabel = new QLabel("Language:", this);
        m_languageCombo = new QComboBox(this);
        m_languageCombo->setEnabled(false);
        auto *languageLayout = new QHBoxLayout();
        languageLayout->addWidget(languageLabel);
        languageLayout->addWidget(m_languageCombo);
        m_rightPanel->addLayout(languageLayout);

        auto *oovLabel = new QLabel("Unknown phonemes:", this);
        m_oovCombo = new QComboBox(this);
        m_oovCombo->addItem("Fail the file", static_cast<int>(OovHandling::Raise));
        m_oovCombo->addItem("Drop the pronunciation", static_cast<int>(OovHandling::Force));
        m_oovCombo->setToolTip("How to treat phonemes that the model vocabulary does not know.");
        auto *oovLayout = new QHBoxLayout();
        oovLayout->addWidget(oovLabel);
        oovLayout->addWidget(m_oovCombo);
        m_rightPanel->addLayout(oovLayout);

        auto *skipLabel = new QLabel("Skipped tokens:", this);
        m_skipCombo = new QComboBox(this);
        m_skipCombo->addItem("Discard the file", static_cast<int>(SkipHandling::Discard));
        m_skipCombo->addItem("Omit the tokens", static_cast<int>(SkipHandling::Omit));
        m_skipCombo->addItem("Preserve as silence", static_cast<int>(SkipHandling::Preserve));
        m_skipCombo->setCurrentIndex(1);
        m_skipCombo->setToolTip("How to write tokens that the decoder could not give any frame.");
        auto *skipLayout = new QHBoxLayout();
        skipLayout->addWidget(skipLabel);
        skipLayout->addWidget(m_skipCombo);
        m_rightPanel->addLayout(skipLayout);

        auto *penaltyLabel = new QLabel("Skip penalty:", this);
        m_skipPenaltySpin = new QDoubleSpinBox(this);
        m_skipPenaltySpin->setRange(0.0, 10.0);
        m_skipPenaltySpin->setSingleStep(0.05);
        m_skipPenaltySpin->setDecimals(3);
        m_skipPenaltySpin->setValue(0.5);
        m_skipPenaltySpin->setToolTip("Cosine score paid by every token without a frame.");
        auto *penaltyLayout = new QHBoxLayout();
        penaltyLayout->addWidget(penaltyLabel);
        penaltyLayout->addWidget(m_skipPenaltySpin);
        m_rightPanel->addLayout(penaltyLayout);

        auto *providerLabel = new QLabel("Execution provider:", this);
        m_providerCombo = new QComboBox(this);
        m_providerCombo->addItem("CPU", static_cast<int>(ExecutionProvider::CPU));
#ifdef ONNXRUNTIME_ENABLE_DML
        m_providerCombo->addItem("DirectML", static_cast<int>(ExecutionProvider::DML));
#endif
        auto *deviceLabel = new QLabel("Device:", this);
        m_deviceCombo = new QComboBox(this);
        m_deviceCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        auto *providerLayout = new QHBoxLayout();
        providerLayout->addWidget(providerLabel);
        providerLayout->addWidget(m_providerCombo);
        providerLayout->addWidget(deviceLabel);
        providerLayout->addWidget(m_deviceCombo);
        connect(m_providerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &MainWindow::slot_updateDeviceList);
        m_rightPanel->addLayout(providerLayout);
        slot_updateDeviceList();

        m_rightPanel->addStretch();
    }

    void MainWindow::loadLanguages() {
        m_languageCombo->clear();
        const std::vector<std::string> available = m_aligner->availableLanguages();
        for (const Language language : kLanguages) {
            const std::string tag = languageTag(language);
            if (std::find(available.begin(), available.end(), tag) == available.end()) {
                continue;
            }
            m_languageCombo->addItem(QString::fromLatin1(languageName(language)), QString::fromStdString(tag));
        }
        m_languageCombo->setEnabled(m_languageCombo->count() > 0);
        if (m_languageCombo->count() == 0) {
            appendErrorMessage("No language is available. Check the dictionary folder.");
        }
    }

    AlignRequest MainWindow::buildRequest() const {
        AlignRequest request;
        request.language = Language::Mandarin;
        const QString tag = m_languageCombo->currentData().toString();
        for (const Language language : kLanguages) {
            if (tag == QString::fromLatin1(languageTag(language))) {
                request.language = language;
                break;
            }
        }
        request.oovHandling = static_cast<OovHandling>(m_oovCombo->currentData().toInt());
        request.skipHandling = static_cast<SkipHandling>(m_skipCombo->currentData().toInt());
        request.skipPenalty = static_cast<float>(m_skipPenaltySpin->value());
        return request;
    }

    void MainWindow::runTask() {
        if (!m_aligner) {
            QMessageBox::warning(this, "Warning", "Model not loaded or invalid. Please load the model first.");
            return;
        }
        const QString outDir = m_outTgEdit->text().trimmed();
        if (outDir.isEmpty()) {
            QMessageBox::warning(this, "Warning", "Output directory cannot be empty.");
            return;
        }
        if (!QDir().exists(outDir)) {
            if (!QDir().mkpath(outDir)) {
                QMessageBox::warning(this, "Warning", "Cannot create output directory. Please check the path.");
                return;
            }
        }
        m_logOutput->clear();
        m_totalTasks = taskList()->count();
        if (m_totalTasks == 0) {
            QMessageBox::information(this, "Info", "No tasks to process.");
            return;
        }
        m_finishedTasks = 0;
        m_errorTasks = 0;
        m_errorDetails.clear();
        m_progressBar->setValue(0);
        m_progressBar->setMaximum(m_totalTasks);

        const AlignRequest request = buildRequest();
        for (int i = 0; i < m_totalTasks; ++i) {
            const QListWidgetItem *item = taskList()->item(i);
            const QString filename = item->text();
            const QString filePath = item->data(Qt::UserRole + 1).toString();
            const QString baseName = QFileInfo(filename).completeBaseName();
            const QString outTgPath = outDir + QDir::separator() + baseName + ".TextGrid";

            auto *thread = new AlignThread(m_aligner, filename, filePath, outTgPath, request);
            connect(thread, &AlignThread::oneFailed, this, &MainWindow::slot_alignFailed);
            connect(thread, &AlignThread::oneFinished, this, &MainWindow::slot_alignFinished);
            m_threadPool->start(thread);
        }
    }

    void MainWindow::onTaskFinished() {
        const QString msg = QString("Alignment completed! Total: %3, Succeeded: %1, Failed: %2")
                                .arg(m_totalTasks - m_errorTasks)
                                .arg(m_errorTasks)
                                .arg(m_totalTasks);
        if (m_errorTasks > 0) {
            m_logOutput->appendPlainText("Failed tasks list:");
            for (const QString &detail : m_errorDetails)
                m_logOutput->appendPlainText("  " + detail);
            m_errorDetails.clear();
        }
        QMessageBox::information(this, QApplication::applicationName(), msg);
        m_totalTasks = m_finishedTasks = m_errorTasks = 0;
    }

    void MainWindow::slot_outTgPath() {
        const QString path =
            QFileDialog::getExistingDirectory(this, "Select Output TextGrid Directory", m_outTgEdit->text(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            m_outTgEdit->setText(path);
    }

    void MainWindow::slot_browseModel() {
        const QString path =
            QFileDialog::getExistingDirectory(this, "Select TIFA Model Folder", m_modelEdit->text(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            m_modelEdit->setText(path);
    }

    void MainWindow::slot_browseDict() {
        const QString path =
            QFileDialog::getExistingDirectory(this, "Select Dictionary Folder", m_dictEdit->text(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            m_dictEdit->setText(path);
    }

    void MainWindow::slot_updateDeviceList() {
        m_deviceCombo->clear();
        const auto provider = static_cast<ExecutionProvider>(m_providerCombo->currentData().toInt());
        if (provider == ExecutionProvider::DML) {
#ifdef _WIN32
            const QList<GpuInfo> gpuList = DmlGpuUtils::getGpuList();
            for (const auto &gpu : gpuList) {
                const double memoryGB = static_cast<double>(gpu.memory) / (1024 * 1024 * 1024);
                const QString deviceName = QString("%1 (%2 GB)").arg(gpu.description).arg(memoryGB, 0, 'f', 2);
                m_deviceCombo->addItem(deviceName, gpu.index);
            }
#endif
        }
        m_deviceCombo->addItem("Default", -1);
    }

    void MainWindow::slot_loadModel() {
        if (m_aligner && m_aligner->initialized()) {
            QMessageBox::information(this, "Info", "Model already loaded, no need to repeat.");
            return;
        }

        const QString modelFolder = m_modelEdit->text().trimmed();
        if (modelFolder.isEmpty()) {
            m_modelStatusLabel->setText("Model folder path is empty.");
            m_runBtn->setEnabled(false);
            return;
        }
        const QString dictFolder = m_dictEdit->text().trimmed();
        if (dictFolder.isEmpty()) {
            m_modelStatusLabel->setText("Dictionary folder path is empty.");
            m_runBtn->setEnabled(false);
            return;
        }

        QString error;
        if (!checkModelFiles(modelFolder, error)) {
            m_modelStatusLabel->setText("Model files missing: " + error);
            m_runBtn->setEnabled(false);
            return;
        }

        delete m_aligner;
        m_aligner = nullptr;

        const auto provider = static_cast<ExecutionProvider>(m_providerCombo->currentData().toInt());
        const int deviceId = m_deviceCombo->currentData().toInt();
        m_aligner = new Aligner();
        std::string message;
        if (!m_aligner->load(toPath(modelFolder), toPath(dictFolder), provider, deviceId, message)) {
            m_modelStatusLabel->setText("Model loading failed: " + QString::fromStdString(message));
            appendErrorMessage(QString::fromStdString(message));
            m_runBtn->setEnabled(false);
            m_modelLoadBtn->setEnabled(true);
            delete m_aligner;
            m_aligner = nullptr;
            return;
        }

        m_modelStatusLabel->setText(provider == ExecutionProvider::CPU ? "Model loaded on CPU."
                                                                       : "Model loaded on the GPU.");
        m_runBtn->setEnabled(true);
        m_modelLoadBtn->setEnabled(false);
        loadLanguages();

        const Config &config = m_aligner->config();
        m_logOutput->appendPlainText(QString("Sample rate %1 Hz, %2 Mel bins, timestep %3 s, vocabulary %4.")
                                         .arg(config.sampleRate)
                                         .arg(config.numMels)
                                         .arg(config.timestep)
                                         .arg(m_aligner->vocabularySize()));
        for (const std::string &warning : m_aligner->warnings()) {
            appendErrorMessage(QString::fromStdString(warning));
        }
    }

    bool MainWindow::checkModelFiles(const QString &modelDir, QString &error) {
        const fs::path folder = toPath(modelDir);
        const fs::path configuration = folder / "config.json";
        if (!fs::exists(configuration)) {
            error = fromPath(configuration) + " does not exist";
            return false;
        }
        const fs::path vocabulary = folder / "vocabulary.json";
        if (!fs::exists(vocabulary)) {
            error = fromPath(vocabulary) + " does not exist";
            return false;
        }
        for (const char *graph : kGraphs) {
            const fs::path path = folder / (std::string(graph) + ".onnx");
            if (!fs::exists(path)) {
                error = fromPath(path) + " does not exist";
                return false;
            }
        }
        return true;
    }

    void MainWindow::slot_alignFailed(const QString &filename, const QString &msg) {
        m_finishedTasks++;
        m_errorTasks++;
        m_errorDetails.append(filename + ": " + msg);
        m_progressBar->setValue(m_finishedTasks);
        appendErrorMessage(filename + ": " + msg);
        if (m_finishedTasks == m_totalTasks) {
            onTaskFinished();
        }
    }

    void MainWindow::slot_alignFinished(const QString &filename, const QString &msg) {
        m_finishedTasks++;
        m_progressBar->setValue(m_finishedTasks);
        if (!msg.isEmpty()) {
            m_logOutput->appendPlainText(filename + ": " + msg);
        }
        if (m_finishedTasks == m_totalTasks) {
            onTaskFinished();
        }
    }

    void MainWindow::appendErrorMessage(const QString &message) const {
        QTextCursor cursor = m_logOutput->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(message + "\n", m_errorFormat);
        m_logOutput->ensureCursorVisible();
    }

} // namespace Tifa
