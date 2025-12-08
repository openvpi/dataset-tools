#include "MainWindow.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QRadioButton>
#include <QStandardPaths>
#include <QStatusBar>

#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "../util/DmlGpuUtils.h"
#include "../util/HfaThread.h"
#include "../util/Provider.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace HFA {
    static bool isDmlAvailable(int &recommendedIndex) {
        const GpuInfo recommendedGpu = DmlGpuUtils::getRecommendedGpu();

        if (recommendedGpu.index == -1)
            return false;

        recommendedIndex = recommendedGpu.index;
        return recommendedGpu.memory > 0;
    }

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

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
        auto rmProvider = ExecutionProvider::CPU;
        int device_id = -1;
        if (isDmlAvailable(device_id)) {
            rmProvider = ExecutionProvider::DML;
            std::cout << "DML is available. Recommended GPU index: " << device_id << std::endl;
        } else {
            std::cout << "DML is not available." << std::endl;
        }

        const QString modelFolder = QDir::cleanPath(
#ifdef Q_OS_MAC
            QApplication::applicationDirPath() + "/../Resources/hfa_model"
#else
            QApplication::applicationDirPath() + QDir::separator() + "hfa_model"
#endif
        );
        if (QDir(modelFolder).exists()) {
            std::string error;
            if (!check_configs(modelFolder.toStdString(), error))
                QMessageBox::information(this, "Warning", QString::fromStdString(error));
            else {
                m_hfa = new HFA(modelFolder.toStdString(), rmProvider, device_id);
            }
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

        const fs::path model_path(modelFolder.toStdString());
        const fs::path vocab_file = model_path / "vocab.json";
        std::ifstream vocab_stream(vocab_file);
        json vocab = json::parse(vocab_stream);

        const auto nonLexicalPh = vocab["non_lexical_phonemes"].get<std::vector<std::string>>();
        const auto languages = vocab["dictionaries"].get<std::map<std::string, std::string>>();

        m_threadpool = new QThreadPool(this);
        m_threadpool->setMaxThreadCount(1);

        setAcceptDrops(true);

        // Init menus
        addFileAction = new QAction("Add File", this);
        addFolderAction = new QAction("Add Folder", this);

        fileMenu = new QMenu("File(&F)", this);
        fileMenu->addAction(addFileAction);
        fileMenu->addAction(addFolderAction);

        aboutAppAction = new QAction(QString("About %1").arg(QApplication::applicationName()), this);
        aboutQtAction = new QAction("About Qt", this);

        helpMenu = new QMenu("Help(&H)", this);
        helpMenu->addAction(aboutAppAction);
        helpMenu->addAction(aboutQtAction);

        const auto bar = menuBar();
        bar->addMenu(fileMenu);
        bar->addMenu(helpMenu);

        listLayout = new QHBoxLayout();

        taskList = new QListWidget();
        taskList->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

        rightLayout = new QVBoxLayout();
        outTgLayout = new QHBoxLayout();

        outTgEdit = new QLineEdit(R"(C:\Users\99662\Desktop\hfa测试wav\wav\TextGrid)");
        const auto btnOutTg = new QPushButton("Open Folder");
        outTgLayout->addWidget(outTgEdit);
        outTgLayout->addWidget(btnOutTg);

        nonSpeechPhLayout = new QHBoxLayout();
        for (const auto &ph : nonLexicalPh) {
            const auto label = new QCheckBox(QString::fromStdString(ph));
            nonSpeechPhLayout->addWidget(label);
        }

        const auto outTgLabel = new QLabel("Out TextGrid Path:");
        const auto nonSpeechPhLabel = new QLabel("Non-speech Phonemes:");

        languageLayout = new QHBoxLayout();
        languageGroup = new QButtonGroup(this);
        languageGroup->setExclusive(true);
        const auto langLabel = new QLabel("Language:");
        int radioId = 0;
        for (auto it = languages.rbegin(); it != languages.rend(); ++it) {
            const auto &[fst, snd] = *it;
            auto *radio = new QRadioButton(QString::fromStdString(fst));
            languageGroup->addButton(radio, radioId);
            languageLayout->addWidget(radio);
            radioId++;
        }
        languageGroup->button(0)->setChecked(true);

        rightLayout->addWidget(outTgLabel);
        rightLayout->addLayout(outTgLayout);
        rightLayout->addWidget(nonSpeechPhLabel);
        rightLayout->addLayout(nonSpeechPhLayout);
        rightLayout->addWidget(langLabel);
        rightLayout->addLayout(languageLayout);
        rightLayout->addStretch(1);

        listLayout->addWidget(taskList, 3);
        listLayout->addLayout(rightLayout, 2);

        btnLayout = new QHBoxLayout();
        remove = new QPushButton("remove");
        clear = new QPushButton("clear");
        runHfa = new QPushButton("runHfa");
        if (!m_hfa)
            runHfa->setDisabled(true);
        btnLayout->addWidget(remove);
        btnLayout->addWidget(clear);
        btnLayout->addWidget(runHfa);

        out = new QPlainTextEdit();
        out->setReadOnly(true);

        progressLabel = new QLabel("progress:");
        progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        progressBar->setValue(0);

        progressLayout = new QHBoxLayout();
        progressLayout->addWidget(progressLabel);
        progressLayout->addWidget(progressBar);

        mainLayout = new QVBoxLayout();
        mainLayout->addLayout(listLayout);
        mainLayout->addLayout(btnLayout);
        mainLayout->addWidget(out);
        mainLayout->addLayout(progressLayout);

        mainWidget = new QWidget();
        mainWidget->setLayout(mainLayout);
        setCentralWidget(mainWidget);

        // Init signals
        connect(fileMenu, &QMenu::triggered, this, &MainWindow::_q_fileMenuTriggered);
        connect(helpMenu, &QMenu::triggered, this, &MainWindow::_q_helpMenuTriggered);

        connect(remove, &QPushButton::clicked, this, &MainWindow::slot_removeListItem);
        connect(clear, &QPushButton::clicked, this, &MainWindow::slot_clearTaskList);
        connect(runHfa, &QPushButton::clicked, this, &MainWindow::slot_runHfa);

        connect(btnOutTg, &QPushButton::clicked, this, &MainWindow::slot_outTgPath);

        resize(960, 720);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::addFiles(const QStringList &paths) const {
        for (const auto &path : paths) {
            const QFileInfo info(path);
            if (info.suffix() == "wav") {
                auto *item = new QListWidgetItem();
                item->setText(info.fileName());
                item->setData(Qt::ItemDataRole::UserRole + 1, path);
                taskList->addItem(item);
            }
        }
    }

    void MainWindow::addFolder(const QString &path) const {
        const QDir dir(path);
        QStringList files = dir.entryList(QDir::Files);
        QStringList absoluteFilePaths;
        for (const QString &file : files) {
            absoluteFilePaths << dir.absoluteFilePath(file);
        }
        addFiles(absoluteFilePaths);
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
        auto urls = event->mimeData()->urls();
        bool has_wav = false;
        bool has_folder = false;
        for (const auto &url : urls) {
            if (url.isLocalFile()) {
                const auto path = url.toLocalFile();
                if (QFileInfo(path).isDir()) {
                    has_folder = true;
                } else {
                    const auto ext = QFileInfo(path).suffix();
                    if (ext.compare("wav", Qt::CaseInsensitive) == 0) {
                        has_wav = true;
                    }
                }
            }
        }
        if (has_wav || has_folder) {
            event->accept();
        }
    }

    void MainWindow::dropEvent(QDropEvent *event) {
        auto urls = event->mimeData()->urls();
        QStringList wavPaths;
        for (const auto &url : urls) {
            if (!url.isLocalFile()) {
                continue;
            }
            const auto path = url.toLocalFile();
            if (QFileInfo(path).isDir())
                addFolder(path);
            else
                wavPaths.append(path);
        }

        if (!wavPaths.isEmpty())
            addFiles(wavPaths);
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        // Quit
        event->accept();
    }

    void MainWindow::_q_fileMenuTriggered(const QAction *action) {
        if (action == addFileAction) {
            const auto paths = QFileDialog::getOpenFileNames(
                this, "Add Files", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "*.wav");
            if (paths.isEmpty()) {
                return;
            }
            addFiles(paths);
        } else if (action == addFolderAction) {
            const QString path = QFileDialog::getExistingDirectory(
                this, "Add Folder", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
            if (path.isEmpty()) {
                return;
            }
            addFolder(path);
        }
    }

    void MainWindow::_q_helpMenuTriggered(const QAction *action) {
        if (action == aboutAppAction) {
            QMessageBox::information(
                this, QApplication::applicationName(),
                QString("%1 %2, Copyright OpenVPI.").arg(QApplication::applicationName(), APP_VERSION));
        } else if (action == aboutQtAction) {
            QMessageBox::aboutQt(this);
        }
    }

    void MainWindow::slot_removeListItem() const {
        auto itemList = taskList->selectedItems();
        for (const auto item : itemList) {
            delete item;
        }
    }

    void MainWindow::slot_clearTaskList() const {
        taskList->clear();
    }

    void MainWindow::slot_runHfa() {
        out->clear();
        m_threadpool->clear();

        const auto outTgDir = outTgEdit->text();
        if (!QDir(outTgDir).exists())
            std::cout << "make path" << outTgDir.toStdString() << QDir(outTgDir).mkpath(".") << std::endl;

        m_workError = 0;
        m_workFinished = 0;
        m_workTotal = taskList->count();
        progressBar->setValue(0);
        progressBar->setMaximum(taskList->count());

        std::vector<std::string> non_speech_ph;
        for (int i = 0; i < nonSpeechPhLayout->count(); i++) {
            const auto checkBox = qobject_cast<QCheckBox *>(nonSpeechPhLayout->itemAt(i)->widget());
            if (checkBox && checkBox->isChecked()) {
                non_speech_ph.push_back(checkBox->text().toStdString());
            }
        }

        const std::string language =
            languageGroup->checkedButton() ? languageGroup->checkedButton()->text().toStdString() : std::string("zh");

        for (int i = 0; i < taskList->count(); i++) {
            const auto item = taskList->item(i);
            const QString outTgPath =
                outTgDir + QDir::separator() + QFileInfo(item->text()).completeBaseName() + ".TextGrid";

            const auto hfaTread = new HfaThread(m_hfa, item->text(), item->data(Qt::UserRole + 1).toString(), outTgPath,
                                                language, non_speech_ph);
            connect(hfaTread, &HfaThread::oneFailed, this, &MainWindow::slot_oneFailed);
            connect(hfaTread, &HfaThread::oneFinished, this, &MainWindow::slot_oneFinished);
            m_threadpool->start(hfaTread);
        }
    }

    void MainWindow::slot_outTgPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Out TextGrid Folder Path", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (path.isEmpty()) {
            return;
        }
        outTgEdit->setText(path);
    }

    void MainWindow::slot_oneFailed(const QString &filename, const QString &msg) {
        m_workFinished++;
        m_workError++;
        m_failIndex.append(filename + ": " + msg);
        progressBar->setValue(m_workFinished);

        out->appendPlainText(filename + ": " + msg);

        if (m_workFinished == m_workTotal) {
            slot_threadFinished();
        }
    }

    void MainWindow::slot_oneFinished(const QString &filename, const QString &msg) {
        m_workFinished++;
        progressBar->setValue(m_workFinished);

        if (!msg.isEmpty())
            out->appendPlainText(filename + ": " + msg);

        if (m_workFinished == m_workTotal) {
            slot_threadFinished();
        }
    }

    void MainWindow::slot_threadFinished() {
        const auto msg = QString("Hfa complete! Total: %3, Success: %1, Failed: %2")
                             .arg(m_workTotal - m_workError)
                             .arg(m_workError)
                             .arg(m_workTotal);
        if (m_workError > 0) {
            QString failSummary = "Failed tasks:\n";
            for (const QString &fileMsg : m_failIndex) {
                failSummary += "  " + fileMsg + "\n";
            }
            out->appendPlainText(failSummary);
            m_failIndex.clear();
        }
        QMessageBox::information(this, QApplication::applicationName(), msg);
        m_workFinished = 0;
        m_workError = 0;
        m_workTotal = 0;
    }
}