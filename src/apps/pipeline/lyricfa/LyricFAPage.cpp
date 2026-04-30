#include "LyricFAPage.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>

#include <dstools/ModelLoadPanel.h>
#include <dstools/PathSelector.h>
#include <dsfw/AsyncTask.h>

#include "Asr.h"
#include "AsrTask.h"
#include "LyricMatchTask.h"
#include "MatchLyric.h"

#include <QDir>
#include <cpp-pinyin/Pinyin.h>

using dstools::widgets::ModelLoadPanel;
using dstools::widgets::PathSelector;

LyricFAPage::LyricFAPage(QWidget *parent) : TaskWindow(PipelineStyle, parent) {
    m_mandarin = QSharedPointer<Pinyin::Pinyin>(new Pinyin::Pinyin());
    m_match = new LyricFA::MatchLyric();
    setRunButtonText("Run ASR");
    setProgressBarVisible(true);
    LyricFAPage::init();
}

LyricFAPage::~LyricFAPage() {
    delete m_asr;
    delete m_match;
}

void LyricFAPage::init() {
    m_labPath = new PathSelector(PathSelector::Directory, "Lab Out Path:", {}, this);
    m_jsonPath = new PathSelector(PathSelector::Directory, "Json Out Path:", {}, this);
    m_lyricPath = new PathSelector(PathSelector::Directory, "Raw Lyric Path:", {}, this);

    m_pinyinBox = new QCheckBox("ASR result saved as pinyin", this);
    m_matchBtn = new QPushButton("Match Lyrics", this);
    connect(m_matchBtn, &QPushButton::clicked, this, &LyricFAPage::slot_matchLyric);

    m_rightPanel->addWidget(m_labPath);
    m_rightPanel->addWidget(m_jsonPath);
    m_rightPanel->addWidget(m_lyricPath);
    m_rightPanel->addWidget(m_pinyinBox);
    m_rightPanel->addWidget(m_matchBtn);
    m_rightPanel->addStretch();

    const QString defaultModelPath = QDir::cleanPath(
#ifdef Q_OS_MAC
        QApplication::applicationDirPath() + "/../Resources/model"
#else
        QApplication::applicationDirPath() + QDir::separator() + "model" + QDir::separator() + "ParaformerAsrModel"
#endif
    );

    m_modelPanel = new ModelLoadPanel(PathSelector::Directory, "ASR Model Folder:", {}, this);
    m_modelPanel->pathSelector()->setPath(defaultModelPath);
    connect(m_modelPanel, &ModelLoadPanel::loadRequested, this, &LyricFAPage::slot_loadModel);
    m_topLayout->addWidget(m_modelPanel);

    slot_loadModel();
}

void LyricFAPage::runTask() {
    if (!m_asr || !m_asr->initialized()) {
        QMessageBox::warning(this, "Warning", "ASR model not loaded.");
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        return;
    }

    m_logOutput->clear();
    threadPool()->clear();

    const QString labOutPath = m_labPath->path();
    if (!QDir(labOutPath).exists()) {
        QMessageBox::information(this, "Warning", "Lab Out Path does not exist.");
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        return;
    }

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
    m_currentMode = Mode_Asr;

    const bool toPinyin = m_pinyinBox->isChecked();

    for (int i = 0; i < m_totalTasks; ++i) {
        const QListWidgetItem *item = taskList()->item(i);
        QString filename = item->text();
        const QString filePath = item->data(Qt::UserRole + 1).toString();
        const QString labFilePath = labOutPath + QDir::separator() + QFileInfo(filename).completeBaseName() + ".lab";

        auto *asrTask = new LyricFA::AsrThread(m_asr, filename, filePath, labFilePath,
                                               QSharedPointer<Pinyin::Pinyin>(toPinyin ? m_mandarin : nullptr));
        connect(asrTask, &dstools::AsyncTask::failed, this, &TaskWindow::slot_oneFailed);
        connect(asrTask, &dstools::AsyncTask::succeeded, this, &TaskWindow::slot_oneFinished);
        threadPool()->start(asrTask);
    }
}

void LyricFAPage::onTaskFinished() {
    const int total = m_totalTasks;
    const int errors = m_errorTasks.load();
    const QString msg = (m_currentMode == Mode_Asr)
                            ? QString("ASR completed! Total: %1, Success: %2, Failed: %3")
                                  .arg(total)
                                  .arg(total - errors)
                                  .arg(errors)
                            : QString("Lyric matching completed! Total: %1, Success: %2, Failed: %3")
                                  .arg(total)
                                  .arg(total - errors)
                                  .arg(errors);

    if (errors > 0) {
        m_logOutput->appendPlainText("Failed tasks:");
        for (const QString &detail : m_errorDetails)
            m_logOutput->appendPlainText("  " + detail);
    }

    QMessageBox::information(this, qApp->applicationName(), msg);
    m_totalTasks = 0;
    m_finishedTasks = 0;
    m_errorTasks = 0;
}

void LyricFAPage::slot_matchLyric() {
    const QString lyricFolder = m_lyricPath->path();
    const QString labFolder = m_labPath->path();
    const QString jsonFolder = m_jsonPath->path();

    if (!QDir(lyricFolder).exists() || !QDir(labFolder).exists() || !QDir(jsonFolder).exists()) {
        QMessageBox::information(this, "Warning", "Please ensure all three paths exist.");
        return;
    }

    m_logOutput->clear();
    threadPool()->clear();
    QDir().mkpath(jsonFolder);

    const QDir labDir(labFolder);
    QStringList labPaths;
    for (const QString &file : labDir.entryList({"*.lab"}, QDir::Files))
        labPaths << labDir.absoluteFilePath(file);

    if (labPaths.isEmpty()) {
        QMessageBox::information(this, "Info", "No .lab files found.");
        return;
    }

    m_match->initLyric(lyricFolder);

    m_totalTasks = labPaths.size();
    m_finishedTasks = 0;
    m_errorTasks = 0;
    m_errorDetails.clear();
    m_progressBar->setValue(0);
    m_progressBar->setMaximum(m_totalTasks);
    m_currentMode = Mode_MatchLyric;
    m_isRunning = true;
    m_runBtn->setEnabled(false);

    for (const QString &labPath : labPaths) {
        QString labName = QFileInfo(labPath).completeBaseName();
        const QString jsonPath = jsonFolder + QDir::separator() + labName + ".json";
        auto *task = new LyricFA::LyricMatchTask(m_match, labName, labPath, jsonPath);
        connect(task, &dstools::AsyncTask::failed, this, &TaskWindow::slot_oneFailed);
        connect(task, &dstools::AsyncTask::succeeded, this, &TaskWindow::slot_oneFinished);
        threadPool()->start(task);
    }
}

void LyricFAPage::slot_loadModel() {
    QString modelFolder = m_modelPanel->path().trimmed();
    if (modelFolder.isEmpty()) {
        m_modelPanel->setStatus("Model folder path is empty.", false);
        m_runBtn->setEnabled(false);
        return;
    }

    QDir dir(modelFolder);
    if (!dir.exists()) {
        m_modelPanel->setStatus("Model folder does not exist.", false);
        m_runBtn->setEnabled(false);
        return;
    }

    if (!QFile::exists(dir.absoluteFilePath("model.onnx")) || !QFile::exists(dir.absoluteFilePath("vocab.txt"))) {
        m_modelPanel->setStatus("Missing model.onnx or vocab.txt", false);
        m_runBtn->setEnabled(false);
        return;
    }

    if (m_asr) {
        delete m_asr;
        m_asr = nullptr;
    }
    m_asr = new LyricFA::Asr(modelFolder);
    if (m_asr->initialized()) {
        m_modelPanel->setStatus("Model loaded successfully.", true);
        m_runBtn->setEnabled(true);
    } else {
        m_modelPanel->setStatus("Failed to load model.", false);
        delete m_asr;
        m_asr = nullptr;
        m_runBtn->setEnabled(false);
    }
}

void LyricFAPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
    if (m_labPath)
        m_labPath->setPath(dir);
}

QString LyricFAPage::workingDirectory() const {
    return m_workingDir;
}

void LyricFAPage::onWorkingDirectoryChanged(const QString &newDir) {
    setWorkingDirectory(newDir);
}
