#include "SlicerPage.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Enumerations.h"
#include "WorkThread.h"

SlicerPage::SlicerPage(QWidget *parent) : QWidget(parent) {
    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(1);
    setupUI();
}

SlicerPage::~SlicerPage() = default;

void SlicerPage::setupUI() {
    auto *mainLayout = new QHBoxLayout(this);

    // Left: file list
    auto *leftLayout = new QVBoxLayout();
    m_fileList = new QListWidget();
    leftLayout->addWidget(m_fileList);

    auto *listBtns = new QHBoxLayout();
    auto *addBtn = new QPushButton(tr("Add Files"));
    auto *addFolderBtn = new QPushButton(tr("Add Folder"));
    auto *removeBtn = new QPushButton(tr("Remove"));
    auto *clearBtn = new QPushButton(tr("Clear"));
    listBtns->addWidget(addBtn);
    listBtns->addWidget(addFolderBtn);
    listBtns->addWidget(removeBtn);
    listBtns->addWidget(clearBtn);
    leftLayout->addLayout(listBtns);

    connect(addBtn, &QPushButton::clicked, this, &SlicerPage::onAddFiles);
    connect(addFolderBtn, &QPushButton::clicked, this, &SlicerPage::onAddFolder);
    connect(removeBtn, &QPushButton::clicked, this, &SlicerPage::onRemoveItem);
    connect(clearBtn, &QPushButton::clicked, this, &SlicerPage::onClearList);

    mainLayout->addLayout(leftLayout, 1);

    // Right: parameters + output
    auto *rightLayout = new QVBoxLayout();

    auto *paramGroup = new QGroupBox(tr("Parameters"));
    auto *paramLayout = new QGridLayout(paramGroup);

    paramLayout->addWidget(new QLabel(tr("Threshold:")), 0, 0);
    m_lineThreshold = new QLineEdit("-40");
    paramLayout->addWidget(m_lineThreshold, 0, 1);

    paramLayout->addWidget(new QLabel(tr("Min Length:")), 1, 0);
    m_lineMinLength = new QLineEdit("5000");
    paramLayout->addWidget(m_lineMinLength, 1, 1);

    paramLayout->addWidget(new QLabel(tr("Min Interval:")), 2, 0);
    m_lineMinInterval = new QLineEdit("300");
    paramLayout->addWidget(m_lineMinInterval, 2, 1);

    paramLayout->addWidget(new QLabel(tr("Hop Size:")), 3, 0);
    m_lineHopSize = new QLineEdit("10");
    paramLayout->addWidget(m_lineHopSize, 3, 1);

    paramLayout->addWidget(new QLabel(tr("Max Silence:")), 4, 0);
    m_lineMaxSilence = new QLineEdit("500");
    paramLayout->addWidget(m_lineMaxSilence, 4, 1);

    paramLayout->addWidget(new QLabel(tr("Output Format:")), 5, 0);
    m_cmbOutputFormat = new QComboBox();
    m_cmbOutputFormat->addItem("16-bit PCM", 0);
    m_cmbOutputFormat->addItem("24-bit PCM", 1);
    m_cmbOutputFormat->addItem("32-bit PCM", 2);
    m_cmbOutputFormat->addItem("32-bit float", 3);
    paramLayout->addWidget(m_cmbOutputFormat, 5, 1);

    paramLayout->addWidget(new QLabel(tr("Slicing Mode:")), 6, 0);
    m_cmbSlicingMode = new QComboBox();
    m_cmbSlicingMode->addItem("Audio Only", QVariant::fromValue(static_cast<int>(SlicingMode::AudioOnly)));
    m_cmbSlicingMode->addItem("Audio + Markers", QVariant::fromValue(static_cast<int>(SlicingMode::AudioAndMarkers)));
    m_cmbSlicingMode->addItem("Markers Only", QVariant::fromValue(static_cast<int>(SlicingMode::MarkersOnly)));
    m_cmbSlicingMode->addItem("Audio (Load Markers)", QVariant::fromValue(static_cast<int>(SlicingMode::AudioOnlyLoadMarkers)));
    paramLayout->addWidget(m_cmbSlicingMode, 6, 1);

    paramLayout->addWidget(new QLabel(tr("Suffix Digits:")), 7, 0);
    m_spnSuffixDigits = new QSpinBox();
    m_spnSuffixDigits->setRange(1, 10);
    m_spnSuffixDigits->setValue(4);
    paramLayout->addWidget(m_spnSuffixDigits, 7, 1);

    m_chkOverwriteMarkers = new QCheckBox(tr("Overwrite Markers"));
    paramLayout->addWidget(m_chkOverwriteMarkers, 8, 0, 1, 2);

    rightLayout->addWidget(paramGroup);

    // Output dir
    auto *outLayout = new QHBoxLayout();
    outLayout->addWidget(new QLabel(tr("Output Dir:")));
    m_lineOutputDir = new QLineEdit();
    outLayout->addWidget(m_lineOutputDir);
    auto *browseBtn = new QPushButton(tr("Browse..."));
    connect(browseBtn, &QPushButton::clicked, this, &SlicerPage::onBrowseOutputDir);
    outLayout->addWidget(browseBtn);
    rightLayout->addLayout(outLayout);

    // Progress + Log + Run
    m_progressBar = new QProgressBar();
    rightLayout->addWidget(m_progressBar);

    m_logOutput = new QTextEdit();
    m_logOutput->setReadOnly(true);
    rightLayout->addWidget(m_logOutput);

    m_btnRun = new QPushButton(tr("Start"));
    connect(m_btnRun, &QPushButton::clicked, this, &SlicerPage::onStart);
    rightLayout->addWidget(m_btnRun);

    mainLayout->addLayout(rightLayout, 2);

    setAcceptDrops(true);
}

void SlicerPage::setProcessing(bool processing) {
    m_processing = processing;
    m_btnRun->setEnabled(!processing);
}

void SlicerPage::logMessage(const QString &txt) {
    m_logOutput->append(txt);
}

void SlicerPage::addSingleAudioFile(const QString &fullPath) {
    QFileInfo info(fullPath);
    if (!info.exists() || info.suffix().toLower() != "wav") return;
    auto *item = new QListWidgetItem(info.fileName());
    item->setData(Qt::UserRole + 1, fullPath);
    m_fileList->addItem(item);
}

void SlicerPage::onBrowseOutputDir() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Browse Output Directory"), ".");
    if (!path.isEmpty()) {
        m_lineOutputDir->setText(QDir::toNativeSeparators(path));
    }
}

void SlicerPage::onAddFiles() {
    if (m_processing) return;
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Select Audio Files"), ".", "Wave Files (*.wav)");
    for (const QString &path : paths) {
        addSingleAudioFile(path);
    }
}

void SlicerPage::onAddFolder() {
    if (m_processing) return;
    QString path = QFileDialog::getExistingDirectory(this, tr("Add Folder"), ".");
    QDir dir(path);
    if (!dir.exists()) return;
    for (const QString &name : dir.entryList({"*.wav"}, QDir::Files)) {
        addSingleAudioFile(path + QDir::separator() + name);
    }
}

void SlicerPage::onRemoveItem() {
    if (m_processing) return;
    qDeleteAll(m_fileList->selectedItems());
}

void SlicerPage::onClearList() {
    if (m_processing) return;
    m_fileList->clear();
}

void SlicerPage::onStart() {
    if (m_processing) return;

    int itemCount = m_fileList->count();
    if (itemCount == 0) return;

    auto slicingMode = static_cast<SlicingMode>(m_cmbSlicingMode->currentData().toInt());
    bool saveAudio = true, saveMarkers = false, loadMarkers = false;
    switch (slicingMode) {
        case SlicingMode::AudioOnlyLoadMarkers: saveAudio = true; loadMarkers = true; break;
        case SlicingMode::MarkersOnly: saveAudio = false; saveMarkers = true; break;
        case SlicingMode::AudioAndMarkers: saveAudio = true; saveMarkers = true; break;
        default: break;
    }
    bool overwriteMarkers = saveMarkers && m_chkOverwriteMarkers->isChecked();

    m_workFinished = 0;
    m_workError = 0;
    m_workTotal = itemCount;
    m_failIndex.clear();
    m_progressBar->setMaximum(itemCount);
    m_progressBar->setValue(0);
    m_logOutput->clear();

    setProcessing(true);
    for (int i = 0; i < itemCount; i++) {
        auto *item = m_fileList->item(i);
        auto path = item->data(Qt::UserRole + 1).toString();
        auto *runnable = new WorkThread(
            path, m_lineOutputDir->text(),
            m_lineThreshold->text().toDouble(),
            m_lineMinLength->text().toLongLong(),
            m_lineMinInterval->text().toLongLong(),
            m_lineHopSize->text().toLongLong(),
            m_lineMaxSilence->text().toLongLong(),
            m_cmbOutputFormat->currentData().toInt(),
            saveAudio, saveMarkers, loadMarkers, overwriteMarkers,
            m_spnSuffixDigits->value(), i);
        connect(runnable, &WorkThread::oneFinished, this, &SlicerPage::onOneFinished);
        connect(runnable, &WorkThread::oneFailed, this, &SlicerPage::onOneFailed);
        m_threadPool->start(runnable);
    }
}

void SlicerPage::onOneFinished(const QString &filename, int listIndex) {
    m_workFinished++;
    m_progressBar->setValue(m_workFinished);
    logMessage(QString("%1 finished.").arg(filename));
    if (m_workFinished == m_workTotal) {
        setProcessing(false);
        logMessage(QString("Complete! Total: %1, Success: %2, Failed: %3")
            .arg(m_workTotal).arg(m_workTotal - m_workError).arg(m_workError));
    }
}

void SlicerPage::onOneFailed(const QString &errmsg, int listIndex) {
    m_workFinished++;
    m_workError++;
    m_failIndex.append(errmsg);
    m_progressBar->setValue(m_workFinished);
    logMessage(QString("[FAILED] %1").arg(errmsg));
    if (m_workFinished == m_workTotal) {
        setProcessing(false);
        logMessage(QString("Complete! Total: %1, Success: %2, Failed: %3")
            .arg(m_workTotal).arg(m_workTotal - m_workError).arg(m_workError));
    }
}
