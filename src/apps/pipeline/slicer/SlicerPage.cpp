#include "SlicerPage.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "Enumerations.h"
#include "WorkThread.h"

SlicerPage::SlicerPage(QWidget *parent) : TaskWindow(PipelineStyle, parent) {
    setMaxThreadCount(1);
    setRunButtonText("Start");
    setProgressBarVisible(true);
    SlicerPage::init();
}

SlicerPage::~SlicerPage() = default;

void SlicerPage::init() {
    auto *paramGroup = new QGroupBox(tr("Parameters"), this);
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

    m_rightPanel->addWidget(paramGroup);

    // Output dir
    auto *outLayout = new QHBoxLayout();
    outLayout->addWidget(new QLabel(tr("Output Dir:")));
    m_lineOutputDir = new QLineEdit();
    outLayout->addWidget(m_lineOutputDir);
    auto *browseBtn = new QPushButton(tr("Browse..."));
    connect(browseBtn, &QPushButton::clicked, this, &SlicerPage::onBrowseOutputDir);
    outLayout->addWidget(browseBtn);
    m_rightPanel->addLayout(outLayout);

    m_rightPanel->addStretch();
}

void SlicerPage::runTask() {
    int itemCount = m_taskListWidget->count();
    if (itemCount == 0) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        return;
    }

    m_isRunning = true;
    m_runBtn->setEnabled(false);

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

    for (int i = 0; i < itemCount; i++) {
        auto *item = m_taskListWidget->item(i);
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
        connect(runnable, &WorkThread::oneInfo, this, &SlicerPage::logMessage);
        connect(runnable, &WorkThread::oneError, this, &SlicerPage::logMessage);
        threadPool()->start(runnable);
    }
}

void SlicerPage::onBrowseOutputDir() {
    QString path = QFileDialog::getExistingDirectory(this, tr("Browse Output Directory"), ".");
    if (!path.isEmpty()) {
        m_lineOutputDir->setText(QDir::toNativeSeparators(path));
    }
}

void SlicerPage::logMessage(const QString &txt) {
    m_logOutput->appendPlainText(txt);
}

void SlicerPage::addSingleAudioFile(const QString &fullPath) {
    QFileInfo info(fullPath);
    if (!info.exists() || info.suffix().toLower() != "wav") return;
    auto *item = new QListWidgetItem(info.fileName());
    item->setData(Qt::UserRole + 1, fullPath);
    m_taskListWidget->addItem(item);
}

void SlicerPage::onOneFinished(const QString &filename, int listIndex) {
    m_workFinished++;
    m_progressBar->setValue(m_workFinished);
    logMessage(QString("%1 finished.").arg(filename));
    if (m_workFinished == m_workTotal) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
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
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        logMessage(QString("Complete! Total: %1, Success: %2, Failed: %3")
            .arg(m_workTotal).arg(m_workTotal - m_workError).arg(m_workError));
    }
}
