#include "HubertFAPage.h"

#include <filesystem>

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QRadioButton>
#include <QTextCursor>

#include <dstools/PathSelector.h>
#include <dstools/ModelLoadPanel.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/AsyncTask.h>
#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ModelManager.h>

namespace fs = std::filesystem;

using dstools::widgets::PathSelector;
using dstools::widgets::ModelLoadPanel;

HubertFAPage::HubertFAPage(QWidget *parent) : TaskWindow(PipelineStyle, parent) {
    m_errorFormat.setForeground(Qt::red);
    setRunButtonText("Run Alignment");
    setProgressBarVisible(true);
    HubertFAPage::init();
    slot_loadModel();
}

HubertFAPage::~HubertFAPage() = default;

void HubertFAPage::init() {
    m_outTgPath = new PathSelector(PathSelector::Directory, "Output TextGrid directory:", {}, this);
    m_rightPanel->addWidget(m_outTgPath);

    const QString defaultModelPath = QDir::cleanPath(
#ifdef Q_OS_MAC
        QApplication::applicationDirPath() + "/../Resources/hfa_model"
#else
        QApplication::applicationDirPath() + QDir::separator() + "model" + QDir::separator() + "HfaModel"
#endif
    );

    m_modelPanel = new ModelLoadPanel(PathSelector::Directory, "HFA model folder:", {}, this);
    m_modelPanel->pathSelector()->setPath(defaultModelPath);
    connect(m_modelPanel, &ModelLoadPanel::loadRequested, this, &HubertFAPage::slot_loadModel);
    m_topLayout->addWidget(m_modelPanel);

    m_rightPanel->addStretch();
}

void HubertFAPage::runTask() {
    if (!m_modelLoaded) {
        QMessageBox::warning(this, "Warning", "Model not loaded.");
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        return;
    }
    const QString outDir = m_outTgPath->path().trimmed();
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

    m_isRunning = true;
    m_runBtn->setEnabled(false);

    m_finishedTasks = 0;
    m_errorTasks = 0;
    m_errorDetails.clear();
    m_progressBar->setValue(0);
    m_progressBar->setMaximum(m_totalTasks);

    for (int i = 0; i < m_totalTasks; ++i) {
        auto *item = taskList()->item(i);
        QString filename = item->text();
        QString filePath = item->data(Qt::UserRole + 1).toString();

        dstools::TaskInput taskInput;
        taskInput.audioPath = filePath;
        auto result = m_processor->process(taskInput);
        if (!result) {
            slot_hfaFailed(filename, QString::fromStdString(result.error()));
        } else {
            slot_hfaFinished(filename, "");
        }
    }
}

void HubertFAPage::onTaskFinished() {
    const int total = m_totalTasks;
    const int errors = m_errorTasks.load();
    QString msg = QString("Alignment completed! Total: %1, Success: %2, Failed: %3")
                      .arg(total).arg(total - errors).arg(errors);
    if (errors > 0) {
        m_logOutput->appendPlainText("Failed tasks:");
        for (const QString &d : m_errorDetails)
            m_logOutput->appendPlainText("  " + d);
    }
    QMessageBox::information(this, qApp->applicationName(), msg);
    m_totalTasks = 0;
    m_finishedTasks = 0;
    m_errorTasks = 0;
}

void HubertFAPage::slot_loadModel() {
    QString modelFolder = m_modelPanel->path().trimmed();
    if (modelFolder.isEmpty()) {
        m_modelPanel->setStatus("Model folder path is empty.", false);
        m_runBtn->setEnabled(false);
        return;
    }

    if (!m_processor) {
        m_processor = dstools::TaskProcessorRegistry::instance().create(
            QStringLiteral("phoneme_alignment"), QStringLiteral("hubert-fa"));
    }
    if (!m_processor) {
        m_modelPanel->setStatus("No alignment processor registered.", false);
        m_runBtn->setEnabled(false);
        return;
    }

    dstools::ModelManager mm;
    dstools::ProcessorConfig config;
    config["path"] = modelFolder.toStdString();
    config["deviceId"] = -1;
    auto loadResult = m_processor->initialize(mm, config);
    if (loadResult) {
        m_modelLoaded = true;
        m_modelPanel->setStatus("Model loaded successfully.", true);
        m_runBtn->setEnabled(true);

        if (!m_dynamicContainer) {
            m_dynamicContainer = new QWidget(this);
            auto *dynLayout = new QVBoxLayout(m_dynamicContainer);

            auto caps = m_processor->capabilities();

            if (caps.contains("nonSpeechPhonemes")) {
                auto &nsPh = caps["nonSpeechPhonemes"];
                if (nsPh.contains("default") && nsPh["default"].is_array()) {
                    dynLayout->addWidget(new QLabel("Non-speech phonemes:", m_dynamicContainer));
                    m_nonSpeechPhLayout = new QHBoxLayout();
                    for (const auto &ph : nsPh["default"]) {
                        if (ph.is_string()) {
                            m_nonSpeechPhLayout->addWidget(
                                new QCheckBox(QString::fromStdString(ph.get<std::string>()), m_dynamicContainer));
                        }
                    }
                    dynLayout->addLayout(m_nonSpeechPhLayout);
                }
            }

            if (caps.contains("language")) {
                auto &lang = caps["language"];
                if (lang.contains("values") && lang["values"].is_array()) {
                    dynLayout->addWidget(new QLabel("Language:", m_dynamicContainer));
                    m_languageGroup = new QButtonGroup(this);
                    m_languageGroup->setExclusive(true);
                    auto *langLayout = new QHBoxLayout();
                    int id = 0;
                    for (auto rit = lang["values"].rbegin(); rit != lang["values"].rend(); ++rit) {
                        auto *radio = new QRadioButton(QString::fromStdString(rit->get<std::string>()), m_dynamicContainer);
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
        m_modelLoaded = false;
        m_modelPanel->setStatus(QString("Model loading failed: %1")
                                    .arg(QString::fromStdString(loadResult.error())), false);
        m_runBtn->setEnabled(false);
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

void HubertFAPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
    if (m_outTgPath)
        m_outTgPath->setPath(dir);
}

QString HubertFAPage::workingDirectory() const {
    return m_workingDir;
}

void HubertFAPage::onWorkingDirectoryChanged(const QString &newDir) {
    setWorkingDirectory(newDir);
}
