#include "HubertFAWindow.h"

#include <filesystem>
#include <fstream>

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>
#include <QStandardPaths>
#include <QTextCursor>

#include <nlohmann/json.hpp>

#include "../util/HfaThread.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace HFA {

    static bool check_configs(const std::string &model_dir, std::string &error) {
        const fs::path model_path(model_dir);
        const fs::path vocab_file = model_path / "vocab.json";
        if (!fs::exists(vocab_file)) {
            error = vocab_file.string() + " does not exist";
            return false;
        }
        const fs::path config_file = model_path / "config.json";
        if (!fs::exists(config_file)) {
            error = config_file.string() + " does not exist";
            return false;
        }
        const fs::path model_file = model_path / "model.onnx";
        if (!fs::exists(model_file)) {
            error = model_file.string() + " does not exist";
            return false;
        }
        std::ifstream vocab_stream(vocab_file);
        json vocab = json::parse(vocab_stream);
        const auto dictionaries = vocab["dictionaries"];
        if (dictionaries.is_object()) {
            for (const auto &[key, dict_node] : dictionaries.items()) {
                if (!dict_node.is_null()) {
                    auto dict_path_str = dict_node.get<std::string>();
                    fs::path dict_path = model_path / dict_path_str;
                    if (!fs::exists(dict_path)) {
                        error = dict_path.string() + " does not exist";
                        return false;
                    }
                }
            }
        }
        return true;
    }

    HubertFAWindow::HubertFAWindow(QWidget *parent)
        : AsyncTaskWindow(parent), m_modelLoadBtn(nullptr), m_dynamicContainer(nullptr) {
        m_errorFormat.setForeground(Qt::red);
        setRunButtonText("Run Alignment");
        HubertFAWindow::init();
        slot_loadModel();
    }

    HubertFAWindow::~HubertFAWindow() {
        delete m_hfa;
    }

    void HubertFAWindow::init() {
        auto *outLabel = new QLabel("Output TextGrid directory:", this);
        m_outTgEdit = new QLineEdit(this);
        m_outTgEdit->setText("");
        auto *outBtn = new QPushButton("Browse...", this);
        auto *outLayout = new QHBoxLayout();
        outLayout->addWidget(outLabel);
        outLayout->addWidget(m_outTgEdit);
        outLayout->addWidget(outBtn);
        connect(outBtn, &QPushButton::clicked, this, &HubertFAWindow::slot_outTgPath);
        m_rightPanel->addLayout(outLayout);

        auto *modelLabel = new QLabel("HFA model folder:", this);
        m_modelEdit = new QLineEdit(this);
        const QString defaultModelPath = QDir::cleanPath(
#ifdef Q_OS_MAC
            QApplication::applicationDirPath() + "/../Resources/hfa_model"
#else
            QApplication::applicationDirPath() + QDir::separator() + "model" + QDir::separator() + "HfaModel"
#endif
        );
        m_modelEdit->setText(defaultModelPath);
        auto *modelBrowseBtn = new QPushButton("Browse...", this);
        m_modelLoadBtn = new QPushButton("Load Model", this);
        m_modelStatusLabel = new QLabel("Model not loaded", this);
        auto *modelLayout = new QHBoxLayout();
        modelLayout->addWidget(modelLabel);
        modelLayout->addWidget(m_modelEdit);
        modelLayout->addWidget(modelBrowseBtn);
        modelLayout->addWidget(m_modelLoadBtn);
        modelLayout->addWidget(m_modelStatusLabel);
        connect(modelBrowseBtn, &QPushButton::clicked, this, &HubertFAWindow::slot_browseModel);
        connect(m_modelLoadBtn, &QPushButton::clicked, this, &HubertFAWindow::slot_loadModel);
        m_topLayout->addLayout(modelLayout);

        m_rightPanel->addStretch();
    }

    void HubertFAWindow::runTask() {
        if (!m_hfa) {
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

        std::vector<std::string> non_speech_ph;
        if (m_nonSpeechPhLayout) {
            for (int i = 0; i < m_nonSpeechPhLayout->count(); ++i) {
                const auto *checkBox = qobject_cast<QCheckBox *>(m_nonSpeechPhLayout->itemAt(i)->widget());
                if (checkBox && checkBox->isChecked()) {
                    non_speech_ph.push_back(checkBox->text().toStdString());
                }
            }
        }
        std::string language = "zh";
        if (m_languageGroup && m_languageGroup->checkedButton()) {
            language = m_languageGroup->checkedButton()->text().toStdString();
        }

        for (int i = 0; i < m_totalTasks; ++i) {
            const QListWidgetItem *item = taskList()->item(i);
            QString filename = item->text();
            QString filePath = item->data(Qt::UserRole + 1).toString();
            QString baseName = QFileInfo(filename).completeBaseName();
            QString outTgPath = outDir + QDir::separator() + baseName + ".TextGrid";

            auto *thread = new HfaThread(m_hfa, filename, filePath, outTgPath, language, non_speech_ph);
            connect(thread, &HfaThread::oneFailed, this, &HubertFAWindow::slot_hfaFailed);
            connect(thread, &HfaThread::oneFinished, this, &HubertFAWindow::slot_hfaFinished);
            m_threadPool->start(thread);
        }
    }

    void HubertFAWindow::onTaskFinished() {
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

    void HubertFAWindow::slot_outTgPath() {
        const QString path =
            QFileDialog::getExistingDirectory(this, "Select Output TextGrid Directory", m_outTgEdit->text(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            m_outTgEdit->setText(path);
    }

    void HubertFAWindow::slot_browseModel() {
        const QString path =
            QFileDialog::getExistingDirectory(this, "Select HFA Model Folder", m_modelEdit->text(),
                                              QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!path.isEmpty())
            m_modelEdit->setText(path);
    }

    void HubertFAWindow::slot_loadModel() {
        if (m_hfa && m_hfa->initialized() && m_dynamicContainer != nullptr) {
            QMessageBox::information(this, "Info", "Model already loaded, no need to repeat.");
            return;
        }

        QString modelFolder = m_modelEdit->text().trimmed();
        if (modelFolder.isEmpty()) {
            m_modelStatusLabel->setText("Model folder path is empty.");
            m_runBtn->setEnabled(false);
            return;
        }

        QString errorStr;
        if (!checkModelConfig(modelFolder, errorStr)) {
            m_modelStatusLabel->setText("Model files missing: " + errorStr);
            m_runBtn->setEnabled(false);
            return;
        }

        if (m_hfa) {
            delete m_hfa;
            m_hfa = nullptr;
        }

        m_hfa = new HFA(modelFolder.toStdString(), ExecutionProvider::CPU, -1);
        if (m_hfa->initialized()) {
            m_modelStatusLabel->setText("Model loaded successfully.");
            m_runBtn->setEnabled(true);
            m_modelLoadBtn->setEnabled(false);

            if (m_dynamicContainer == nullptr) {
                m_dynamicContainer = new QWidget(this);
                auto *dynamicLayout = new QVBoxLayout(m_dynamicContainer);

                fs::path model_path(modelFolder.toStdString());
                fs::path vocab_file = model_path / "vocab.json";
                std::ifstream vocab_stream(vocab_file);
                json vocab = json::parse(vocab_stream);

                if (vocab.contains("non_lexical_phonemes")) {
                    auto nonLexicalPh = vocab["non_lexical_phonemes"].get<std::vector<std::string>>();
                    if (!nonLexicalPh.empty()) {
                        auto *phLabel = new QLabel("Non-speech phonemes:", m_dynamicContainer);
                        dynamicLayout->addWidget(phLabel);

                        m_nonSpeechPhLayout = new QHBoxLayout();
                        for (const auto &ph : nonLexicalPh) {
                            auto *checkBox = new QCheckBox(QString::fromStdString(ph), m_dynamicContainer);
                            m_nonSpeechPhLayout->addWidget(checkBox);
                        }
                        dynamicLayout->addLayout(m_nonSpeechPhLayout);
                    }
                }

                if (vocab.contains("dictionaries")) {
                    auto languages = vocab["dictionaries"].get<std::map<std::string, std::string>>();
                    if (!languages.empty()) {
                        auto *langLabel = new QLabel("Language:", m_dynamicContainer);
                        dynamicLayout->addWidget(langLabel);

                        m_languageGroup = new QButtonGroup(this);
                        m_languageGroup->setExclusive(true);
                        auto *langLayout = new QHBoxLayout();
                        int radioId = 0;
                        for (auto it = languages.rbegin(); it != languages.rend(); ++it) {
                            auto *radio = new QRadioButton(QString::fromStdString(it->first), m_dynamicContainer);
                            m_languageGroup->addButton(radio, radioId++);
                            langLayout->addWidget(radio);
                        }
                        if (!langLayout->isEmpty())
                            m_languageGroup->button(0)->setChecked(true);
                        dynamicLayout->addLayout(langLayout);
                    }
                }

                int stretchIndex = m_rightPanel->count() - 1;
                m_rightPanel->insertWidget(stretchIndex, m_dynamicContainer);
            }
        } else {
            m_modelStatusLabel->setText("Model loading failed (initialization error).");
            m_runBtn->setEnabled(false);
            m_modelLoadBtn->setEnabled(true);
        }
    }

    bool HubertFAWindow::checkModelConfig(const QString &modelDir, QString &error) {
        std::string err;
        const bool ok = check_configs(modelDir.toStdString(), err);
        if (!ok) {
            error = QString::fromStdString(err);
        }
        return ok;
    }

    void HubertFAWindow::slot_hfaFailed(const QString &filename, const QString &msg) {
        m_finishedTasks++;
        m_errorTasks++;
        m_errorDetails.append(filename + ": " + msg);
        m_progressBar->setValue(m_finishedTasks);
        appendErrorMessage(filename + ": " + msg);
        if (m_finishedTasks == m_totalTasks) {
            onTaskFinished();
        }
    }

    void HubertFAWindow::slot_hfaFinished(const QString &filename, const QString &msg) {
        m_finishedTasks++;
        m_progressBar->setValue(m_finishedTasks);
        if (!msg.isEmpty()) {
            m_logOutput->appendPlainText(filename + ": " + msg);
        }
        if (m_finishedTasks == m_totalTasks) {
            onTaskFinished();
        }
    }

    void HubertFAWindow::appendErrorMessage(const QString &message) const {
        QTextCursor cursor = m_logOutput->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(message + "\n", m_errorFormat);
        m_logOutput->ensureCursorVisible();
    }

}