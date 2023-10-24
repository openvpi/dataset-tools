#include <cmath>

#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QList>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRunnable>
#include <QScreen>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QSysInfo>
#include <QTextStream>
#include <QThreadPool>
#include <QValidator>


#include "enumerations.h"
#include "mainwindow.h"
#include "mainwindow_ui.h"
#include "workthread.h"

#ifdef Q_OS_WIN
#include "utils/winfont.h"
#endif


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    initStylesMenu();

    m_threadpool = new QThreadPool(this);
    m_threadpool->setMaxThreadCount(1);


    connect(ui->btnAddFiles, &QPushButton::clicked, this, &MainWindow::slot_addAudioFiles);
    connect(ui->btnBrowse, &QPushButton::clicked, this, &MainWindow::slot_browseOutputDir);
    connect(ui->btnRemoveListItem, &QPushButton::clicked, this, &MainWindow::slot_removeListItem);
    connect(ui->btnClearList, &QPushButton::clicked, this, &MainWindow::slot_clearAudioList);
    connect(ui->pushButtonAbout, &QPushButton::clicked, this, &MainWindow::slot_about);
    connect(ui->pushButtonStart, &QPushButton::clicked, this, &MainWindow::slot_start);
    connect(ui->actionAddFile, &QAction::triggered, this, &MainWindow::slot_addAudioFiles);
    connect(ui->actionAddFolder, &QAction::triggered, this, &MainWindow::slot_addFolder);
    connect(ui->actionSaveLogs, &QAction::triggered, this, &MainWindow::slot_saveLogs);
    connect(ui->actionAboutApp, &QAction::triggered, this, &MainWindow::slot_about);
    connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::slot_aboutQt);
    connect(ui->actionQuit, &QAction::triggered, this, &QCoreApplication::quit, Qt::QueuedConnection);
    connect(ui->actionGroupThemes, &QActionGroup::triggered, [](QAction *action) {
        QApplication::setStyle(action->text());
    });
    connect(ui->cmbSlicingMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::slot_slicingModeChanged);

    slot_slicingModeChanged(ui->cmbSlicingMode->currentIndex());

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setValue(0);

    auto validator = new QRegularExpressionValidator(QRegularExpression("\\d+"));
    ui->lineEditThreshold->setValidator(new QDoubleValidator());
    ui->lineEditMinLen->setValidator(validator);
    ui->lineEditMinInterval->setValidator(validator);
    ui->lineEditHopSize->setValidator(validator);
    ui->lineEditMaxSilence->setValidator(validator);

    ui->cmbOutputWaveFormat->addItem("16-bit integer PCM", QVariant::fromValue(static_cast<int>(WF_INT16_PCM)));
    ui->cmbOutputWaveFormat->addItem("24-bit integer PCM", QVariant::fromValue(static_cast<int>(WF_INT24_PCM)));
    ui->cmbOutputWaveFormat->addItem("32-bit integer PCM", QVariant::fromValue(static_cast<int>(WF_INT32_PCM)));
    ui->cmbOutputWaveFormat->addItem("32-bit float", QVariant::fromValue(static_cast<int>(WF_FLOAT32)));

    m_workTotal = 0;
    m_workFinished = 0;
    m_workError = 0;
    m_processing = false;
    m_windowLoaded = false;

    setWindowTitle(qApp->applicationName());
    setAcceptDrops(true);

#ifdef Q_OS_WIN
    m_hwnd = (HWND) this->winId();
    ::CoInitialize(nullptr);

    HRESULT hr = ::CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER, IID_ITaskbarList3,
                                    (void **) &m_pTaskbarList3);

    if (hr != S_OK) {
        m_pTaskbarList3 = nullptr;
    }
#endif
}

MainWindow::~MainWindow() {
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState(m_hwnd, TBPF_NOPROGRESS);
        m_pTaskbarList3->Release();
        m_pTaskbarList3 = nullptr;
    }
    ::CoUninitialize();
#endif
    delete ui;
}

void MainWindow::slot_browseOutputDir() {
    QString path = QFileDialog::getExistingDirectory(this, "Browse Output Directory", ".");
    if (!path.isEmpty()) {
        ui->lineEditOutputDir->setText(QDir::toNativeSeparators(path));
    }
}

void MainWindow::slot_addAudioFiles() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    QStringList paths = QFileDialog::getOpenFileNames(this, "Select Audio Files", ".", "Wave Files (*.wav)");
    for (const QString &path : paths) {
        addSingleAudioFile(path);
    }
}

void MainWindow::slot_addFolder() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }
    QString path = QFileDialog::getExistingDirectory(this, "Add Folder", ".");
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }
    QStringList files = dir.entryList({"*.wav"}, QDir::Files);
    for (const QString &name : files) {
        QString fullPath = path + QDir::separator() + name;
        addSingleAudioFile(fullPath);
    }
}

void MainWindow::slot_removeListItem() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    auto itemList = ui->listWidgetTaskList->selectedItems();
    for (auto item : itemList) {
        delete item;
    }
}

void MainWindow::slot_clearAudioList() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    ui->listWidgetTaskList->clear();
}

void MainWindow::slot_about() {
    QString arch = QSysInfo::currentCpuArchitecture();
    // QString aboutMsg = QString(
    //     "<p><b>About Audio Slicer</b></p>"
    //     "<p>"
    //         "Arch: %1<br>"
    //         "Qt Version: %2<br>"
    //         "Git branch: %3<br>"
    //         "Git commit: %4<br>"
    //         "Build date: %5<br>"
    //         "Toolchain: %6"
    //     "</p>"
    //     "<p><b>Copyright 2019-%7 OpenVPI.</b></p>")
    //         .arg(arch,
    //              QT_VERSION_STR,
    //              CHORUSKIT_GIT_BRANCH,
    //              QString(CHORUSKIT_GIT_COMMIT_HASH).left(7),
    //              CHORUSKIT_BUILD_DATE_TIME,
    //              CHORUSKIT_BUILD_COMPILER_ID,
    //              CHORUSKIT_BUILD_YEAR);


    QMessageBox::information(this, qApp->applicationName(),
                             QString("%1 %2 (%3), Copyright OpenVPI.").arg(
                                 qApp->applicationName(), APP_VERSION, arch));

    // QMessageBox::information(this, "About", aboutMsg);
}

void MainWindow::slot_aboutQt() {
    QMessageBox::aboutQt(this);
}

void MainWindow::slot_start() {
    if (m_processing) {
        warningProcessNotFinished();
        return;
    }

    auto slicingMode = ui->cmbSlicingMode->currentData().value<SlicingMode>();
    bool saveAudio, saveMarkers, loadMarkers;
    switch (slicingMode) {
        case SlicingMode::AudioOnlyLoadMarkers:
            saveAudio = true;
            saveMarkers = false;
            loadMarkers = true;
            break;
        case SlicingMode::MarkersOnly:
            saveAudio = false;
            saveMarkers = true;
            loadMarkers = false;
            break;
        case SlicingMode::AudioAndMarkers:
            saveAudio = true;
            saveMarkers = true;
            loadMarkers = false;
            break;
        case SlicingMode::AudioOnly:
        default:
            saveAudio = true;
            saveMarkers = false;
            loadMarkers = false;
            break;
    }
    bool overwriteMarkers = saveMarkers && ui->cbOverwriteMarkers->isChecked();
    if (!(saveAudio || saveMarkers)) {
        QMessageBox::warning(this, qApp->applicationName(), "Must save audio files or save markers!");
        return;
    }

    int item_count = ui->listWidgetTaskList->count();
    if (item_count == 0) {
        return;
    }

    m_workFinished = 0;
    m_workError = 0;
    m_workTotal = item_count;

    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(item_count);
    ui->progressBar->setValue(0);

    ui->txtLogs->clear();

    m_failIndex.clear();

#ifdef Q_OS_WIN
    // Show taskbar progress (Windows 7 and later)
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState(m_hwnd, TBPF_NORMAL);
        m_pTaskbarList3->SetProgressValue(m_hwnd, (ULONGLONG) 0, (ULONGLONG) m_workTotal);
    }
#endif

    setProcessing(true);
    for (int i = 0; i < item_count; i++) {
        auto item = ui->listWidgetTaskList->item(i);
        item->setForeground(QBrush());
        item->setBackground(QBrush());
        auto path = item->data(Qt::ItemDataRole::UserRole + 1).toString();
        auto runnable =
            new WorkThread(path, ui->lineEditOutputDir->text(),
                           ui->lineEditThreshold->text().toDouble(),
                           ui->lineEditMinLen->text().toLongLong(),
                           ui->lineEditMinInterval->text().toLongLong(),
                           ui->lineEditHopSize->text().toLongLong(),
                           ui->lineEditMaxSilence->text().toLongLong(),
                           ui->cmbOutputWaveFormat->currentData().toInt(),
                           saveAudio,
                           saveMarkers,
                           loadMarkers,
                           overwriteMarkers,
                           i);
        connect(runnable, &WorkThread::oneFinished, this, &MainWindow::slot_oneFinished);
        connect(runnable, &WorkThread::oneInfo, this, &MainWindow::slot_oneInfo);
        connect(runnable, &WorkThread::oneError, this, &MainWindow::slot_oneError);
        connect(runnable, &WorkThread::oneFailed, this, &MainWindow::slot_oneFailed);
        m_threadpool->start(runnable);
    }
}

void MainWindow::slot_saveLogs() {
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString filename = QString("slicer-%1.log").arg(timestamp);
    QString path = QFileDialog::getSaveFileName(this, "Save Log File", filename, "Log File (*.log)");
    if (path.isEmpty()) {
        return;
    }
    QFile writeFile(path);
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Can't write output log file!");
    } else {
        QTextStream textStream(&writeFile);
        QString txt = ui->txtLogs->toPlainText();
        textStream << txt;
    }
    writeFile.close();
}

void MainWindow::slot_oneFinished(const QString &filename, int listIndex) {
    m_workFinished++;
    ui->progressBar->setValue(m_workFinished);
    logMessage(QString("%1 finished.").arg(filename));

    if (listIndex >= 0) {
        auto item = ui->listWidgetTaskList->item(listIndex);
        item->setForeground(QBrush());
        item->setBackground(QBrush());
    }
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState((HWND) this->winId(), TBPF_NORMAL);
        m_pTaskbarList3->SetProgressValue((HWND) this->winId(), (ULONGLONG) m_workFinished, (ULONGLONG) m_workTotal);
    }
#endif

    if (m_workFinished == m_workTotal) {
        slot_threadFinished();
    }
}

void MainWindow::slot_oneInfo(const QString &infomsg) {
    logMessage(infomsg);
}

void MainWindow::slot_oneError(const QString &errmsg) {
    logMessage("[ERROR] " + errmsg);
}

void MainWindow::slot_oneFailed(const QString &filename, int listIndex) {
    m_workFinished++;
    m_workError++;
    m_failIndex.append(filename);
    logMessage(QString("%1 failed.").arg(filename));
    ui->progressBar->setValue(m_workFinished);

    if (listIndex >= 0) {
        auto item = ui->listWidgetTaskList->item(listIndex);
        item->setForeground(QColor(0x9c, 0x00, 0x06));
        item->setBackground(QColor(0xff, 0xc7, 0xce));
    }
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState((HWND) this->winId(), TBPF_NORMAL);
        m_pTaskbarList3->SetProgressValue((HWND) this->winId(), (ULONGLONG) m_workFinished, (ULONGLONG) m_workTotal);
    }
#endif
    // QMessageBox::critical(this, "Error", errmsg);

    if (m_workFinished == m_workTotal) {
        slot_threadFinished();
    }
}

void MainWindow::slot_threadFinished() {
    setProcessing(false);
    auto msg = QString("Slicing complete! Total: %3, Success: %1, Failed: %2")
                   .arg(m_workTotal - m_workError)
                   .arg(m_workError)
                   .arg(m_workTotal);
    logMessage(msg);
    QString failSummary;
    if (m_workError > 0) {
        failSummary = "Failed tasks:\n";
        for (const QString &filename : m_failIndex) {
            failSummary += "  " + filename + "\n";
        }
        logMessage(failSummary);
        m_failIndex.clear();
    }
#ifdef Q_OS_WIN
    if (m_pTaskbarList3) {
        m_pTaskbarList3->SetProgressState((HWND) this->winId(), TBPF_NOPROGRESS);
    }
#endif
    QMessageBox::information(this, qApp->applicationName(), msg);
    m_workFinished = 0;
    m_workError = 0;
    m_workTotal = 0;
}

void MainWindow::warningProcessNotFinished() {
    QMessageBox::warning(this, qApp->applicationName(), "Please wait for slicing to complete!");
}

void MainWindow::setProcessing(bool processing) {
    bool enabled = !processing;
    ui->pushButtonStart->setText(processing ? "Slicing..." : "Start");
    ui->pushButtonStart->setEnabled(enabled);
    ui->btnAddFiles->setEnabled(enabled);
    ui->listWidgetTaskList->setEnabled(enabled);
    ui->btnRemoveListItem->setEnabled(enabled);
    ui->btnClearList->setEnabled(enabled);
    ui->lineEditThreshold->setEnabled(enabled);
    ui->lineEditMinLen->setEnabled(enabled);
    ui->lineEditMinInterval->setEnabled(enabled);
    ui->lineEditHopSize->setEnabled(enabled);
    ui->lineEditMaxSilence->setEnabled(enabled);
    ui->lineEditOutputDir->setEnabled(enabled);
    ui->btnBrowse->setEnabled(enabled);
    ui->cmbOutputWaveFormat->setEnabled(enabled);
    ui->cmbSlicingMode->setEnabled(enabled);
    ui->cbOverwriteMarkers->setEnabled(enabled);
    ui->actionAddFile->setEnabled(enabled);
    ui->actionAddFolder->setEnabled(enabled);
    m_processing = processing;
}

void MainWindow::logMessage(const QString &txt) {
    if (!txt.isEmpty()) {
        auto timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        ui->txtLogs->append(QString("[%1] %2").arg(timestamp, txt));
    }
}

void MainWindow::addSingleAudioFile(const QString &fullPath) {
    auto *item = new QListWidgetItem();
    item->setText(QFileInfo(fullPath).fileName());
    item->setData(Qt::ItemDataRole::UserRole + 1, fullPath);
    ui->listWidgetTaskList->addItem(item);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_processing) {
        warningProcessNotFinished();
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    auto urls = event->mimeData()->urls();
    bool has_wav = false;
    for (const auto &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        auto path = url.toLocalFile();
        auto ext = QFileInfo(path).suffix();
        if (ext.compare("wav", Qt::CaseInsensitive) == 0) {
            has_wav = true;
            break;
        }
    }
    if (has_wav) {
        event->accept();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    auto urls = event->mimeData()->urls();
    for (const auto &url : urls) {
        if (!url.isLocalFile()) {
            continue;
        }
        auto path = url.toLocalFile();
        auto ext = QFileInfo(path).suffix();
        if (ext.compare("wav", Qt::CaseInsensitive) != 0) {
            continue;
        }

        addSingleAudioFile(path);
    }
}

void MainWindow::initStylesMenu() {
    auto availableStyles = QStyleFactory::keys();
    for (const QString &style : availableStyles) {
        auto styleAction = new QAction(style, ui->menuViewThemes);
        styleAction->setCheckable(true);
        ui->actionGroupThemes->addAction(styleAction);
        ui->menuViewThemes->addAction(styleAction);
    }
    if (!availableStyles.isEmpty()) {
        auto firstStyleAction = ui->actionGroupThemes->actions().first();
        firstStyleAction->setChecked(true);
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    if (!m_windowLoaded) {
        m_windowLoaded = true;
#ifdef Q_OS_MAC
        auto dpiScale = 1.0;
        // It seems that on macOS, logicalDotsPerInch() always return 72.
        // Therefore, we just don't resize on macOS.
#else
        // Get current screen
        auto currentScreen = screen();
        // Try calculate screen DPI scale factor. If currentScreen is nullptr, use 1.0.
        auto dpiScale = currentScreen ? currentScreen->logicalDotsPerInch() / 96.0 : 1.0;
        // Get current window size
        auto oldSize = size();
        // Calculate DPI scaled window size
        auto newSize = dpiScale * oldSize;
        // Calculate window move offset
        auto sizeDiff = (newSize - oldSize) / 2;
        // Resize window to DPI scaled size
        resize(newSize);
        // Move window so its center stays almost the same position
        move(pos().x() - sizeDiff.width(), pos().y() - sizeDiff.height());
#endif
        auto splitterMainSizes = ui->splitterMain->sizes();

        if (ui->splitterMain->count() >= 2) {
            ui->splitterMain->setStretchFactor(0, 1);
            ui->splitterMain->setStretchFactor(1, 0);
            auto firstTwoColumnSizes = splitterMainSizes[0] + splitterMainSizes[1];
            splitterMainSizes[0] = static_cast<int>(std::round(1.0 * 3 * firstTwoColumnSizes / 5));
            splitterMainSizes[1] = static_cast<int>(std::round(1.0 * 2 * firstTwoColumnSizes / 5));
            ui->splitterMain->setSizes(splitterMainSizes);
        }
    }
}

void MainWindow::slot_slicingModeChanged(int index) {
    auto slicingMode = ui->cmbOutputWaveFormat->itemData(index).value<SlicingMode>();
    if ((slicingMode == SlicingMode::MarkersOnly) || (slicingMode == SlicingMode::AudioAndMarkers)) {
        ui->cbOverwriteMarkers->setVisible(true);
    } else {
        ui->cbOverwriteMarkers->setVisible(false);
    }
}

#ifdef Q_OS_WIN
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, long *result) {
    if (!message) {
        // If the message pointer is nullptr, just return false and let Qt handle it.
        return false;
    }

    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        // If the message is Windows native message
        MSG *msg = static_cast<MSG *>(message);
        if ((msg->message == WM_SETTINGCHANGE) && (msg->wParam == SPI_SETNONCLIENTMETRICS)) {
            // System non-client metrics change, including font change
            // Set app font to current system font
            setWinFont();
        }
    }
    return false;
}
#endif
