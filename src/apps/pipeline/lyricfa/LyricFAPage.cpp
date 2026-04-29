#include "LyricFAPage.h"

#include <QApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStandardPaths>

#include "AsrTask.h"
#include "LyricMatchTask.h"
#include "Asr.h"
#include "MatchLyric.h"

#include <cpp-pinyin/Pinyin.h>

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
    auto *labLabel = new QLabel("Lab Out Path:", this);
    m_labEdit = new QLineEdit("", this);
    auto *btnLab = new QPushButton("Open Folder", this);
    auto *labLayout = new QHBoxLayout();
    labLayout->addWidget(m_labEdit);
    labLayout->addWidget(btnLab);

    auto *jsonLabel = new QLabel("Json Out Path:", this);
    m_jsonEdit = new QLineEdit("", this);
    auto *btnJson = new QPushButton("Open Folder", this);
    auto *jsonLayout = new QHBoxLayout();
    jsonLayout->addWidget(m_jsonEdit);
    jsonLayout->addWidget(btnJson);

    auto *lyricLabel = new QLabel("Raw Lyric Path:", this);
    m_lyricEdit = new QLineEdit("", this);
    auto *btnLyric = new QPushButton("Lyric Folder", this);
    auto *lyricLayout = new QHBoxLayout();
    lyricLayout->addWidget(m_lyricEdit);
    lyricLayout->addWidget(btnLyric);

    m_pinyinBox = new QCheckBox("ASR result saved as pinyin", this);
    m_matchBtn = new QPushButton("Match Lyrics", this);
    connect(m_matchBtn, &QPushButton::clicked, this, &LyricFAPage::slot_matchLyric);

    m_rightPanel->addWidget(labLabel);
    m_rightPanel->addLayout(labLayout);
    m_rightPanel->addWidget(jsonLabel);
    m_rightPanel->addLayout(jsonLayout);
    m_rightPanel->addWidget(lyricLabel);
    m_rightPanel->addLayout(lyricLayout);
    m_rightPanel->addWidget(m_pinyinBox);
    m_rightPanel->addWidget(m_matchBtn);
    m_rightPanel->addStretch();

    connect(btnLab, &QPushButton::clicked, this, &LyricFAPage::slot_labPath);
    connect(btnJson, &QPushButton::clicked, this, &LyricFAPage::slot_jsonPath);
    connect(btnLyric, &QPushButton::clicked, this, &LyricFAPage::slot_lyricPath);

    const QString defaultModelPath = QDir::cleanPath(
#ifdef Q_OS_MAC
        QApplication::applicationDirPath() + "/../Resources/model"
#else
        QApplication::applicationDirPath() + QDir::separator() + "model" + QDir::separator() + "ParaformerAsrModel"
#endif
    );

    auto *modelLabel = new QLabel("ASR Model Folder:", this);
    m_modelEdit = new QLineEdit(this);
    m_modelEdit->setText(defaultModelPath);
    auto *browseBtn = new QPushButton("Browse...", this);
    auto *loadBtn = new QPushButton("Load Model", this);
    m_modelStatusLabel = new QLabel("Model not loaded", this);

    auto *loadModelLayout = new QHBoxLayout();
    loadModelLayout->addWidget(modelLabel);
    loadModelLayout->addWidget(m_modelEdit);
    loadModelLayout->addWidget(browseBtn);
    loadModelLayout->addWidget(loadBtn);
    loadModelLayout->addWidget(m_modelStatusLabel);
    m_topLayout->addLayout(loadModelLayout);

    connect(browseBtn, &QPushButton::clicked, this, &LyricFAPage::slot_browseModel);
    connect(loadBtn, &QPushButton::clicked, this, &LyricFAPage::slot_loadModel);

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

    const QString labOutPath = m_labEdit->text();
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
        connect(asrTask, &LyricFA::AsrThread::oneFailed, this, &TaskWindow::slot_oneFailed);
        connect(asrTask, &LyricFA::AsrThread::oneFinished, this, &TaskWindow::slot_oneFinished);
        threadPool()->start(asrTask);
    }
}

void LyricFAPage::onTaskFinished() {
    QString msg = (m_currentMode == Mode_Asr)
        ? QString("ASR completed! Total: %1, Success: %2, Failed: %3")
              .arg(m_totalTasks).arg(m_totalTasks - m_errorTasks).arg(m_errorTasks)
        : QString("Lyric matching completed! Total: %1, Success: %2, Failed: %3")
              .arg(m_totalTasks).arg(m_totalTasks - m_errorTasks).arg(m_errorTasks);

    if (m_errorTasks > 0) {
        m_logOutput->appendPlainText("Failed tasks:");
        for (const QString &detail : m_errorDetails)
            m_logOutput->appendPlainText("  " + detail);
    }

    QMessageBox::information(this, qApp->applicationName(), msg);
    m_totalTasks = m_finishedTasks = m_errorTasks = 0;
}

void LyricFAPage::slot_matchLyric() {
    const QString lyricFolder = m_lyricEdit->text();
    const QString labFolder = m_labEdit->text();
    const QString jsonFolder = m_jsonEdit->text();

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
         connect(task, &LyricFA::LyricMatchTask::oneFailed, this, &TaskWindow::slot_oneFailed);
         connect(task, &LyricFA::LyricMatchTask::oneFinished, this, &TaskWindow::slot_oneFinished);
        threadPool()->start(task);
    }
}

void LyricFAPage::slot_labPath() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Lab Output Directory");
    if (!path.isEmpty()) m_labEdit->setText(path);
}

void LyricFAPage::slot_jsonPath() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Json Output Directory");
    if (!path.isEmpty()) m_jsonEdit->setText(path);
}

void LyricFAPage::slot_lyricPath() {
    QString path = QFileDialog::getExistingDirectory(this, "Select Raw Lyric Directory");
    if (!path.isEmpty()) m_lyricEdit->setText(path);
}

void LyricFAPage::slot_browseModel() {
    QString path = QFileDialog::getExistingDirectory(this, "Select ASR Model Folder", m_modelEdit->text());
    if (!path.isEmpty()) m_modelEdit->setText(path);
}

void LyricFAPage::slot_loadModel() {
    QString modelFolder = m_modelEdit->text().trimmed();
    if (modelFolder.isEmpty()) {
        m_modelStatusLabel->setText("Model folder path is empty.");
        m_runBtn->setEnabled(false);
        return;
    }

    QDir dir(modelFolder);
    if (!dir.exists()) {
        m_modelStatusLabel->setText("Model folder does not exist.");
        m_runBtn->setEnabled(false);
        return;
    }

    if (!QFile::exists(dir.absoluteFilePath("model.onnx")) || !QFile::exists(dir.absoluteFilePath("vocab.txt"))) {
        m_modelStatusLabel->setText("Missing model.onnx or vocab.txt");
        m_runBtn->setEnabled(false);
        return;
    }

    if (m_asr) { delete m_asr; m_asr = nullptr; }
    m_asr = new LyricFA::Asr(modelFolder);
    if (m_asr->initialized()) {
        m_modelStatusLabel->setText("Model loaded successfully.");
        m_runBtn->setEnabled(true);
    } else {
        m_modelStatusLabel->setText("Failed to load model.");
        delete m_asr; m_asr = nullptr;
        m_runBtn->setEnabled(false);
    }
}
