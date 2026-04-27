#include "HubertFAPage.h"

#include <filesystem>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRadioButton>
#include <QTextCursor>

#include <dstools/JsonHelper.h>

// Include original HFA headers directly since they haven't moved to hubert-infer yet
#include "../../HubertFA/util/Hfa.h"
#include "../../HubertFA/util/HfaThread.h"

namespace fs = std::filesystem;

HubertFAPage::HubertFAPage(QWidget *parent) : TaskWindow(PipelineStyle, parent) {
    m_errorFormat.setForeground(Qt::red);
    setRunButtonText("Run Alignment");
    setProgressBarVisible(true);
    HubertFAPage::init();
    slot_loadModel();
}

HubertFAPage::~HubertFAPage() {
    delete m_hfa;
}

void HubertFAPage::init() {
    auto *outLabel = new QLabel("Output TextGrid directory:", this);
    m_outTgEdit = new QLineEdit(this);
    auto *outBtn = new QPushButton("Browse...", this);
    auto *outLayout = new QHBoxLayout();
    outLayout->addWidget(outLabel);
    outLayout->addWidget(m_outTgEdit);
    outLayout->addWidget(outBtn);
    connect(outBtn, &QPushButton::clicked, this, &HubertFAPage::slot_outTgPath);
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
    connect(modelBrowseBtn, &QPushButton::clicked, this, &HubertFAPage::slot_browseModel);
    connect(m_modelLoadBtn, &QPushButton::clicked, this, &HubertFAPage::slot_loadModel);
    m_topLayout->addLayout(modelLayout);

    m_rightPanel->addStretch();
}

void HubertFAPage::runTask() {
    if (!m_hfa) {
        QMessageBox::warning(this, "Warning", "Model not loaded.");
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        return;
    }
    const QString outDir = m_outTgEdit->text().trimmed();
    if (outDir.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Output directory cannot be empty.");
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        return;
    }
    QDir().mkpath(outDir);

    m_logOutput->clear();
    m_totalTasks = taskList()->count();
    if (m_totalTasks == 0) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
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
            auto *cb = qobject_cast<QCheckBox *>(m_nonSpeechPhLayout->itemAt(i)->widget());
            if (cb && cb->isChecked())
                non_speech_ph.push_back(cb->text().toStdString());
        }
    }
    std::string language = "zh";
    if (m_languageGroup && m_languageGroup->checkedButton())
        language = m_languageGroup->checkedButton()->text().toStdString();

    for (int i = 0; i < m_totalTasks; ++i) {
        auto *item = taskList()->item(i);
        QString filename = item->text();
        QString filePath = item->data(Qt::UserRole + 1).toString();
        QString baseName = QFileInfo(filename).completeBaseName();
        QString outTgPath = outDir + QDir::separator() + baseName + ".TextGrid";

        auto *thread = new HFA::HfaThread(m_hfa, filename, filePath, outTgPath, language, non_speech_ph);
        connect(thread, &HFA::HfaThread::oneFailed, this, &HubertFAPage::slot_hfaFailed);
        connect(thread, &HFA::HfaThread::oneFinished, this, &HubertFAPage::slot_hfaFinished);
        threadPool()->start(thread);
    }
}

void HubertFAPage::onTaskFinished() {
    QString msg = QString("Alignment completed! Total: %1, Success: %2, Failed: %3")
                      .arg(m_totalTasks).arg(m_totalTasks - m_errorTasks).arg(m_errorTasks);
    if (m_errorTasks > 0) {
        m_logOutput->appendPlainText("Failed tasks:");
        for (const QString &d : m_errorDetails)
            m_logOutput->appendPlainText("  " + d);
    }
    QMessageBox::information(this, qApp->applicationName(), msg);
    m_totalTasks = m_finishedTasks = m_errorTasks = 0;
}

void HubertFAPage::slot_outTgPath() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Output TextGrid Directory", m_outTgEdit->text());
    if (!path.isEmpty()) m_outTgEdit->setText(path);
}

void HubertFAPage::slot_browseModel() {
    QString path = QFileDialog::getExistingDirectory(this, "Select HFA Model Folder", m_modelEdit->text());
    if (!path.isEmpty()) m_modelEdit->setText(path);
}

void HubertFAPage::slot_loadModel() {
    QString modelFolder = m_modelEdit->text().trimmed();
    if (modelFolder.isEmpty()) {
        m_modelStatusLabel->setText("Model folder path is empty.");
        m_runBtn->setEnabled(false);
        return;
    }

    if (m_hfa) { delete m_hfa; m_hfa = nullptr; }
    m_hfa = new HFA::HFA(modelFolder.toStdString(), HFA::ExecutionProvider::CPU, -1);
    if (m_hfa->initialized()) {
        m_modelStatusLabel->setText("Model loaded successfully.");
        m_runBtn->setEnabled(true);
        m_modelLoadBtn->setEnabled(false);

        // Build dynamic UI from vocab.json
        if (!m_dynamicContainer) {
            m_dynamicContainer = new QWidget(this);
            auto *dynLayout = new QVBoxLayout(m_dynamicContainer);

            fs::path mp(modelFolder.toStdString());
            fs::path vocabFile = mp / "vocab.json";
            std::string jsonErr;
            auto vocab = dstools::JsonHelper::loadFile(vocabFile, jsonErr);
            if (!jsonErr.empty()) vocab = nlohmann::json::object();

            {
                auto nps = dstools::JsonHelper::getVec<std::string>(vocab, "non_lexical_phonemes");
                if (!nps.empty()) {
                    dynLayout->addWidget(new QLabel("Non-speech phonemes:", m_dynamicContainer));
                    m_nonSpeechPhLayout = new QHBoxLayout();
                    for (const auto &ph : nps) {
                        m_nonSpeechPhLayout->addWidget(new QCheckBox(QString::fromStdString(ph), m_dynamicContainer));
                    }
                    dynLayout->addLayout(m_nonSpeechPhLayout);
                }
            }

            {
                auto langs = dstools::JsonHelper::getMap<std::string, std::string>(vocab, "dictionaries");
                if (!langs.empty()) {
                    dynLayout->addWidget(new QLabel("Language:", m_dynamicContainer));
                    m_languageGroup = new QButtonGroup(this);
                    m_languageGroup->setExclusive(true);
                    auto *langLayout = new QHBoxLayout();
                    int id = 0;
                    for (auto it = langs.rbegin(); it != langs.rend(); ++it) {
                        auto *radio = new QRadioButton(QString::fromStdString(it->first), m_dynamicContainer);
                        m_languageGroup->addButton(radio, id++);
                        langLayout->addWidget(radio);
                    }
                    if (m_languageGroup->button(0))
                        m_languageGroup->button(0)->setChecked(true);
                    dynLayout->addLayout(langLayout);
                }
            }

            int stretchIdx = m_rightPanel->count() - 1;
            m_rightPanel->insertWidget(stretchIdx, m_dynamicContainer);
        }
    } else {
        m_modelStatusLabel->setText("Model loading failed.");
        m_runBtn->setEnabled(false);
        m_modelLoadBtn->setEnabled(true);
    }
}

void HubertFAPage::slot_hfaFailed(const QString &filename, const QString &msg) {
    m_finishedTasks++;
    m_errorTasks++;
    m_errorDetails.append(filename + ": " + msg);
    m_progressBar->setValue(m_finishedTasks);
    QTextCursor cursor = m_logOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(filename + ": " + msg + "\n", m_errorFormat);
    m_logOutput->ensureCursorVisible();
    if (m_finishedTasks == m_totalTasks) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        onTaskFinished();
        emit allTasksFinished();
    }
}

void HubertFAPage::slot_hfaFinished(const QString &filename, const QString &msg) {
    m_finishedTasks++;
    m_progressBar->setValue(m_finishedTasks);
    if (!msg.isEmpty()) m_logOutput->appendPlainText(filename + ": " + msg);
    if (m_finishedTasks == m_totalTasks) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        onTaskFinished();
        emit allTasksFinished();
    }
}
