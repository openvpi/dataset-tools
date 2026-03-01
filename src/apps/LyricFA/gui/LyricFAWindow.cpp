#include "LyricFAWindow.h"

#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QStandardPaths>

#include "../util/AsrThread.h"
#include "../util/FaTread.h"

namespace LyricFA {

    LyricFAWindow::LyricFAWindow(QWidget *parent) : AsyncTaskWindow(parent) {
        const QString modelFolder = QDir::cleanPath(
#ifdef Q_OS_MAC
            QApplication::applicationDirPath() + "/../Resources/model"
#else
            QApplication::applicationDirPath() + QDir::separator() + "model"
#endif
        );
        if (QDir(modelFolder).exists()) {
            const auto modelPath = modelFolder + QDir::separator() + "model.onnx";
            const auto vocabPath = modelFolder + QDir::separator() + "vocab.txt";
            if (!QFile(modelPath).exists() || !QFile(vocabPath).exists())
                QMessageBox::information(
                    this, "Warning",
                    "Missing model.onnx or vocab.txt, please read ReadMe.md and download the model again.");
            else
                m_asr = new Asr(modelFolder);
        } else {
#ifdef Q_OS_MAC
            QMessageBox::information(
                this, "Warning",
                "Please read ReadMe.md and download asrModel to unzip to app bundle's Resources directory.");
#else
            QMessageBox::information(this, "Warning",
                                     "Please read ReadMe.md and download asrModel to unzip to the root directory.");
#endif
        }

        m_mandarin = QSharedPointer<Pinyin::Pinyin>(new Pinyin::Pinyin());
        m_match = new MatchLyric();

        setRunButtonText("运行 ASR");
        LyricFAWindow::init();
    }

    LyricFAWindow::~LyricFAWindow() {
        delete m_asr;
        delete m_match;
    }

    void LyricFAWindow::init() {
        auto *labLabel = new QLabel("Lab Out Path:", this);
        m_labEdit = new QLineEdit(R"(D:\python\LyricFA\test_outlab)", this);
        auto *btnLab = new QPushButton("Open Folder", this);
        auto *labLayout = new QHBoxLayout();
        labLayout->addWidget(m_labEdit);
        labLayout->addWidget(btnLab);

        auto *jsonLabel = new QLabel("Json Out Path:", this);
        m_jsonEdit = new QLineEdit(R"(D:\python\LyricFA\test_outjson)", this);
        auto *btnJson = new QPushButton("Open Folder", this);
        auto *jsonLayout = new QHBoxLayout();
        jsonLayout->addWidget(m_jsonEdit);
        jsonLayout->addWidget(btnJson);

        auto *lyricLabel = new QLabel("Raw Lyric Path:", this);
        m_lyricEdit = new QLineEdit(R"(D:\python\LyricFA\lyrics)", this);
        auto *btnLyric = new QPushButton("Lyric Folder", this);
        auto *lyricLayout = new QHBoxLayout();
        lyricLayout->addWidget(m_lyricEdit);
        lyricLayout->addWidget(btnLyric);

        m_pinyinBox = new QCheckBox("ASR result saved as pinyin", this);
        m_matchBtn = new QPushButton("匹配歌词", this);
        connect(m_matchBtn, &QPushButton::clicked, this, &LyricFAWindow::slot_matchLyric);

        m_rightPanel->addWidget(labLabel);
        m_rightPanel->addLayout(labLayout);
        m_rightPanel->addWidget(jsonLabel);
        m_rightPanel->addLayout(jsonLayout);
        m_rightPanel->addWidget(lyricLabel);
        m_rightPanel->addLayout(lyricLayout);
        m_rightPanel->addWidget(m_pinyinBox);
        m_rightPanel->addWidget(m_matchBtn);
        m_rightPanel->addStretch();

        connect(btnLab, &QPushButton::clicked, this, &LyricFAWindow::slot_labPath);
        connect(btnJson, &QPushButton::clicked, this, &LyricFAWindow::slot_jsonPath);
        connect(btnLyric, &QPushButton::clicked, this, &LyricFAWindow::slot_lyricPath);
    }

    void LyricFAWindow::runTask() {
        m_logOutput->clear();
        m_threadPool->clear();

        const QString labOutPath = m_labEdit->text();
        if (!QDir(labOutPath).exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Lab Out Path is empty or does not exist. Please set the output directory.");
            return;
        }

        m_totalTasks = taskList()->count();
        if (m_totalTasks == 0) {
            QMessageBox::information(this, "Info", "No tasks to run.");
            return;
        }
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
            const QString labFilePath =
                labOutPath + QDir::separator() + QFileInfo(filename).completeBaseName() + ".lab";

            auto *asrThread = new AsrThread(m_asr, filename, filePath, labFilePath,
                                            QSharedPointer<Pinyin::Pinyin>(toPinyin ? m_mandarin.data() : nullptr));
            connect(asrThread, &AsrThread::oneFailed, this, &AsyncTaskWindow::slot_oneFailed);
            connect(asrThread, &AsrThread::oneFinished, this, &AsyncTaskWindow::slot_oneFinished);
            m_threadPool->start(asrThread);
        }
    }

    void LyricFAWindow::onTaskFinished() {
        QString msg;
        if (m_currentMode == Mode_Asr) {
            msg = QString("ASR 任务完成！总数: %3, 成功: %1, 失败: %2")
                      .arg(m_totalTasks - m_errorTasks)
                      .arg(m_errorTasks)
                      .arg(m_totalTasks);
        } else {
            msg = QString("歌词匹配完成！总数: %3, 成功: %1, 失败: %2")
                      .arg(m_totalTasks - m_errorTasks)
                      .arg(m_errorTasks)
                      .arg(m_totalTasks);
        }

        if (m_errorTasks > 0) {
            m_logOutput->appendPlainText("失败任务列表：");
            for (const QString &detail : m_errorDetails)
                m_logOutput->appendPlainText("  " + detail);
            m_errorDetails.clear();
        }

        QMessageBox::information(this, QApplication::applicationName(), msg);
        m_totalTasks = m_finishedTasks = m_errorTasks = 0;
    }

    void LyricFAWindow::slot_matchLyric() {
        const QString lyricFolder = m_lyricEdit->text();
        const QString labFolder = m_labEdit->text();
        const QString jsonFolder = m_jsonEdit->text();

        if (!QDir(lyricFolder).exists() || !QDir(labFolder).exists() || !QDir(jsonFolder).exists()) {
            QMessageBox::information(nullptr, "Warning", "Please ensure all three paths exist and are set correctly.");
            return;
        }

        m_logOutput->clear();
        m_threadPool->clear();

        QDir().mkpath(jsonFolder);

        const QDir labDir(labFolder);
        QStringList labPaths;
        for (const QString &file : labDir.entryList(QStringList() << "*.lab", QDir::Files)) {
            labPaths << labDir.absoluteFilePath(file);
        }

        if (labPaths.isEmpty()) {
            QMessageBox::information(this, "Info", "No .lab files found in the lab folder.");
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

        for (const QString &labPath : labPaths) {
            QString labName = QFileInfo(labPath).completeBaseName();
            const QString jsonPath = jsonFolder + QDir::separator() + labName + ".json";

            auto *faThread = new FaTread(m_match, labName, labPath, jsonPath);
            connect(faThread, &FaTread::oneFailed, this, &AsyncTaskWindow::slot_oneFailed);
            connect(faThread, &FaTread::oneFinished, this, &AsyncTaskWindow::slot_oneFinished);
            m_threadPool->start(faThread);
        }
    }

    void LyricFAWindow::slot_labPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Select Lab Output Directory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (!path.isEmpty())
            m_labEdit->setText(path);
    }

    void LyricFAWindow::slot_jsonPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Select Json Output Directory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (!path.isEmpty())
            m_jsonEdit->setText(path);
    }

    void LyricFAWindow::slot_lyricPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Select Raw Lyric Directory", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (!path.isEmpty())
            m_lyricEdit->setText(path);
    }

} // namespace LyricFA