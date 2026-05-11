#include "SlicerPage.h"
#include "AudacityMarkerIO.h"
#include "AudioFileListPanel.h"
#include "SliceCommands.h"
#include "SliceExportDialog.h"
#include "SliceListPanel.h"

#include <ui/WaveformChartPanel.h>
#include <ui/SpectrogramChartPanel.h>
#include <ui/PowerChartPanel.h>
#include <ui/SliceBoundaryModel.h>

#include <dsfw/Log.h>

#include <dsfw/widgets/FileProgressTracker.h>
#include <dsfw/AppSettings.h>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>
#include <audio-util/Slicer.h>

#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>
#include <QSplitter>
#include <QVBoxLayout>

#include <sndfile.hh>

#include <algorithm>
#include <cmath>

namespace dstools {

SlicerPage::SlicerPage(QWidget *parent) : ChartPageBase(QStringLiteral("Slicer"), parent), m_settings("Slicer") {
    m_undoStack = new QUndoStack(this);
    m_boundaryModel = new phonemelabeler::SliceBoundaryModel();
    buildLayout();
    connectSignals();
}

SlicerPage::~SlicerPage() = default;

void SlicerPage::buildLayout() {
    m_audioFileList = new AudioFileListPanel(this);
    m_audioFileList->setMinimumWidth(160);
    m_audioFileList->setMaximumWidth(280);
    m_audioFileList->setShowProgress(true);
    m_audioFileList->progressTracker()->setDisplayStyle(dsfw::widgets::FileProgressTracker::ProgressBarStyle);
    m_audioFileList->progressTracker()->setFormat(QStringLiteral("%1 / %2 已标注"));

    auto *contentWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    m_toolbar = new QToolBar(contentWidget);
    m_toolbar->setIconSize(QSize(20, 20));

    m_btnPointer = new QToolButton(m_toolbar);
    m_btnPointer->setText(QStringLiteral("↖ 指针"));
    m_btnPointer->setToolTip(QStringLiteral("选择/拖动切线 (V)"));
    m_btnPointer->setCheckable(true);
    m_btnPointer->setChecked(true);
    m_btnPointer->setShortcut(QKeySequence(Qt::Key_V));

    m_btnKnife = new QToolButton(m_toolbar);
    m_btnKnife->setText(QStringLiteral("✂ 切刀"));
    m_btnKnife->setToolTip(QStringLiteral("添加切线 (C)"));
    m_btnKnife->setCheckable(true);
    m_btnKnife->setShortcut(QKeySequence(Qt::Key_C));

    m_toolbar->addWidget(m_btnPointer);
    m_toolbar->addWidget(m_btnKnife);
    m_toolbar->addSeparator();

    mainLayout->addWidget(m_toolbar);

    auto *paramsWidget = new QWidget(contentWidget);
    auto *paramsLayout = new QHBoxLayout(paramsWidget);
    paramsLayout->setContentsMargins(4, 2, 4, 2);
    paramsLayout->setSpacing(6);

    auto addParam = [&](const QString &label, QWidget *spin) {
        auto *lbl = new QLabel(label, paramsWidget);
        lbl->setStyleSheet(QStringLiteral("color: #aaa; font-size: 11px;"));
        paramsLayout->addWidget(lbl);
        paramsLayout->addWidget(spin);
    };

    m_thresholdSpin = new QDoubleSpinBox(paramsWidget);
    m_thresholdSpin->setRange(-60.0, 0.0);
    m_thresholdSpin->setValue(-40.0);
    m_thresholdSpin->setSuffix(QStringLiteral(" dB"));
    m_thresholdSpin->setFixedWidth(90);
    addParam(QStringLiteral("阈值"), m_thresholdSpin);

    m_minLengthSpin = new QSpinBox(paramsWidget);
    m_minLengthSpin->setRange(500, 60000);
    m_minLengthSpin->setValue(5000);
    m_minLengthSpin->setSuffix(QStringLiteral(" ms"));
    m_minLengthSpin->setFixedWidth(90);
    addParam(QStringLiteral("长度"), m_minLengthSpin);

    m_minIntervalSpin = new QSpinBox(paramsWidget);
    m_minIntervalSpin->setRange(100, 5000);
    m_minIntervalSpin->setValue(300);
    m_minIntervalSpin->setSuffix(QStringLiteral(" ms"));
    m_minIntervalSpin->setFixedWidth(80);
    addParam(QStringLiteral("间隔"), m_minIntervalSpin);

    m_hopSizeSpin = new QSpinBox(paramsWidget);
    m_hopSizeSpin->setRange(1, 100);
    m_hopSizeSpin->setValue(10);
    m_hopSizeSpin->setSuffix(QStringLiteral(" ms"));
    m_hopSizeSpin->setFixedWidth(70);
    addParam(QStringLiteral("Hop"), m_hopSizeSpin);

    m_maxSilenceSpin = new QSpinBox(paramsWidget);
    m_maxSilenceSpin->setRange(100, 10000);
    m_maxSilenceSpin->setValue(500);
    m_maxSilenceSpin->setSuffix(QStringLiteral(" ms"));
    m_maxSilenceSpin->setFixedWidth(80);
    addParam(QStringLiteral("静音"), m_maxSilenceSpin);

    paramsLayout->addSpacing(8);

    m_btnImportMarkers = new QPushButton(QStringLiteral("导入"), paramsWidget);
    m_btnAutoSlice = new QPushButton(QStringLiteral("切片"), paramsWidget);
    m_btnReSlice = new QPushButton(QStringLiteral("重切"), paramsWidget);
    m_btnSaveMarkers = new QPushButton(QStringLiteral("保存"), paramsWidget);
    m_btnExportAudio = new QPushButton(QStringLiteral("导出..."), paramsWidget);

    paramsLayout->addWidget(m_btnImportMarkers);
    paramsLayout->addWidget(m_btnAutoSlice);
    paramsLayout->addWidget(m_btnReSlice);
    paramsLayout->addWidget(m_btnSaveMarkers);
    paramsLayout->addWidget(m_btnExportAudio);
    paramsLayout->addStretch();

    mainLayout->addWidget(paramsWidget);

    setupContainerAndPlayWidget(3000);

    m_tierLabel = new SliceTierLabel(m_container);
    m_tierLabel->setViewportController(m_viewport);
    m_container->setTierLabelArea(m_tierLabel);

    addWaveformChart(0, 1, 2.0);
    m_waveformChart->setBoundaryModel(m_boundaryModel);

    addSpectrogramChart(1, 1, 5.0);
    m_spectrogramChart->setBoundaryModel(m_boundaryModel);
    m_spectrogramChart->setVisible(true);

    // Power chart: registered but hidden by default (D-30).
    // Slicer users primarily need waveform + spectrogram; power can be
    // toggled on via Settings if desired.
    addPowerChart(2, 1, 3.0);
    m_powerChart->hide();
    m_container->setChartVisible(QStringLiteral("power"), false);

    m_container->setBoundaryModel(m_boundaryModel);

    mainLayout->addWidget(m_container, 1);

    m_sliceListPanel = new SliceListPanel(contentWidget);
    m_sliceListPanel->setSlicerMode(true);
    m_sliceListPanel->setMaximumHeight(200);
    mainLayout->addWidget(m_sliceListPanel);

    m_hSplitter = new QSplitter(Qt::Horizontal, this);
    m_hSplitter->addWidget(m_audioFileList);
    m_hSplitter->addWidget(contentWidget);
    m_hSplitter->setStretchFactor(0, 0);
    m_hSplitter->setStretchFactor(1, 1);

    auto *topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addWidget(m_hSplitter);
}

void SlicerPage::connectSignals() {
    // Viewport connections are managed by AudioVisualizerContainer via
    // connectViewportToWidget (called by addChart). Direct connections
    // here would cause duplicate setViewport() calls.
    connect(m_btnAutoSlice, &QPushButton::clicked, this, &SlicerPage::onAutoSlice);
    connect(m_btnReSlice, &QPushButton::clicked, this, &SlicerPage::onAutoSlice);
    connect(m_btnImportMarkers, &QPushButton::clicked, this, &SlicerPage::onImportMarkers);
    connect(m_btnSaveMarkers, &QPushButton::clicked, this, &SlicerPage::onSaveMarkers);
    connect(m_btnExportAudio, &QPushButton::clicked, this, &SlicerPage::onExportAudio);

    connect(m_btnPointer, &QToolButton::clicked, this, [this]() {
        m_btnPointer->setChecked(true);
        m_btnKnife->setChecked(false);
        m_toolMode = ToolMode::Pointer;
    });
    connect(m_btnKnife, &QToolButton::clicked, this, [this]() {
        m_btnKnife->setChecked(true);
        m_btnPointer->setChecked(false);
        m_toolMode = ToolMode::Knife;
    });

    connect(m_audioFileList, &AudioFileListPanel::fileSelected, this, [this](const QString &filePath) {
        saveCurrentSlicePoints();
        loadAudioFile(filePath);
        m_currentAudioPath = filePath;
        m_undoStack->clear();
        loadSlicePointsForFile(filePath);

        if (m_slicePoints.empty() && !m_samples.empty()) {
            onAutoSlice();
        }

        refreshBoundaries();
        updateSlicerListPanel();
        updateFileProgress();
    });

    connect(m_audioFileList, &AudioFileListPanel::filesAdded, this,
            [this](const QStringList &paths) { autoSliceFiles(paths); });
    connect(m_audioFileList, &AudioFileListPanel::filesRemoved, this, [this]() { updateFileProgress(); });

    connect(m_waveformChart, &phonemelabeler::WaveformChartPanel::positionClicked, this, [this](double sec) {
        if (m_toolMode == ToolMode::Knife) {
            auto refreshFn = [this]() {
                refreshBoundaries();
                updateSlicerListPanel();
            };
            m_undoStack->push(new AddSlicePointCommand(m_slicePoints, sec, refreshFn));
        }
    });

    connect(m_waveformChart, &phonemelabeler::WaveformChartPanel::boundaryDragFinished,
            this, [this](int, int boundaryIndex, dstools::TimePos) {
        if (boundaryIndex >= 0 && boundaryIndex < static_cast<int>(m_slicePoints.size())) {
            m_slicePoints = m_boundaryModel->slicePointsSec();
            refreshBoundaries();
            updateSlicerListPanel();
        }
    });

    connect(m_sliceListPanel, &SliceListPanel::sliceDoubleClicked, this,
            [this](int, double startSec, double) {
                double viewDuration = m_viewport->state().endSec - m_viewport->state().startSec;
                m_viewport->setViewRange(startSec, startSec + viewDuration);
            });

    connect(m_sliceListPanel, &SliceListPanel::addSlicePointRequested, this, [this](double timeSec) {
        auto refreshFn = [this]() {
            refreshBoundaries();
            updateSlicerListPanel();
        };
        m_undoStack->push(new AddSlicePointCommand(m_slicePoints, timeSec, refreshFn));
    });

    connect(m_sliceListPanel, &SliceListPanel::removeSlicePointRequested, this, [this](int pointIndex) {
        if (pointIndex >= 0 && pointIndex < static_cast<int>(m_slicePoints.size())) {
            auto refreshFn = [this]() {
                refreshBoundaries();
                updateSlicerListPanel();
            };
            m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, pointIndex, refreshFn));
        }
    });
}

void SlicerPage::onAutoSlice() {
    if (m_samples.empty())
        return;

    int sr = m_sampleRate;

    AudioUtil::SlicerParams params;
    params.threshold = m_thresholdSpin->value();
    params.minLength = m_minLengthSpin->value();
    params.minInterval = m_minIntervalSpin->value();
    params.hopSize = m_hopSizeSpin->value();
    params.maxSilKept = m_maxSilenceSpin->value();

    auto slicer = AudioUtil::Slicer::fromMilliseconds(sr, params);
    auto markers = slicer.slice(m_samples);

    std::vector<double> newPoints;
    for (size_t i = 0; i + 1 < markers.size(); ++i) {
        int64_t boundary = markers[i].second;
        if (markers[i + 1].first > boundary)
            boundary = (boundary + markers[i + 1].first) / 2;
        double cutTime = static_cast<double>(boundary) / sr;
        newPoints.push_back(cutTime);
    }

    m_undoStack->beginMacro(tr("Auto slice"));
    while (!m_slicePoints.empty()) {
        m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, static_cast<int>(m_slicePoints.size()) - 1,
                                                      [this]() { refreshBoundaries(); }));
    }
    for (double t : newPoints) {
        m_undoStack->push(new AddSlicePointCommand(m_slicePoints, t, [this]() { refreshBoundaries(); }));
    }
    m_undoStack->endMacro();

    refreshBoundaries();
    updateSlicerListPanel();
    saveCurrentSlicePoints();
    updateFileProgress();
    DSFW_LOG_INFO("audio", ("Auto-slice completed: " + std::to_string(newPoints.size()) + " slice points").c_str());
}

void SlicerPage::onImportMarkers() {
    QString path =
        QFileDialog::getOpenFileName(this, tr("Import Markers"), {}, tr("Audacity Labels (*.txt);;All Files (*)"));
    if (path.isEmpty())
        return;

    QString error;
    auto times = AudacityMarkerIO::read(path, error);
    if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Import Error"), error);
        return;
    }

    m_undoStack->beginMacro(tr("Import markers"));
    while (!m_slicePoints.empty()) {
        m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, static_cast<int>(m_slicePoints.size()) - 1,
                                                      [this]() { refreshBoundaries(); }));
    }
    for (double t : times) {
        m_undoStack->push(new AddSlicePointCommand(m_slicePoints, t, [this]() { refreshBoundaries(); }));
    }
    m_undoStack->endMacro();
    refreshBoundaries();
    updateSlicerListPanel();
    saveCurrentSlicePoints();
    updateFileProgress();
}

void SlicerPage::onSaveMarkers() {
    QString path =
        QFileDialog::getSaveFileName(this, tr("Save Markers"), {}, tr("Audacity Labels (*.txt);;All Files (*)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!AudacityMarkerIO::write(path, m_slicePoints, error))
        QMessageBox::warning(this, tr("Save Error"), error);
}

void SlicerPage::onExportAudio() {
    if (m_slicePoints.empty() || m_samples.empty()) {
        QMessageBox::information(this, tr("Export"), tr("No slices to export. Run auto-slice first."));
        return;
    }

    SliceExportDialog dlg(this);
    QString currentAudioPath = m_audioFileList->currentFilePath();
    QFileInfo audioInfo(currentAudioPath);
    dlg.setDefaultPrefix(audioInfo.completeBaseName());
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString outputDir = dlg.outputDir();
    if (outputDir.isEmpty()) {
        QMessageBox::warning(this, tr("Export"), tr("No output directory selected."));
        return;
    }

    QDir dir(outputDir);
    if (!dir.exists())
        dir.mkpath(outputDir);

    const auto &samples = m_samples;
    int sr = m_sampleRate;
    QString prefix = dlg.prefix();
    int digits = dlg.numDigits();

    std::vector<std::pair<int, int>> segments;
    int numSegments = static_cast<int>(m_slicePoints.size()) + 1;
    for (int i = 0; i < numSegments; ++i) {
        double startSec = (i == 0) ? 0.0 : m_slicePoints[i - 1];
        double endSec = (i < static_cast<int>(m_slicePoints.size())) ? m_slicePoints[i]
                                                                     : static_cast<double>(samples.size()) / sr;
        int startSamp = static_cast<int>(startSec * sr);
        int endSamp = std::min(static_cast<int>(endSec * sr), static_cast<int>(samples.size()));
        segments.emplace_back(startSamp, endSamp);
    }

    int sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    switch (dlg.bitDepth()) {
        case SliceExportDialog::BitDepth::PCM16:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
            break;
        case SliceExportDialog::BitDepth::PCM24:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
            break;
        case SliceExportDialog::BitDepth::PCM32:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
            break;
        case SliceExportDialog::BitDepth::Float32:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            break;
    }

    int exported = 0;
    for (int i = 0; i < numSegments; ++i) {
        auto [start, end] = segments[i];
        if (end <= start)
            continue;

        QString filename = QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0'));
        QString filepath = dir.filePath(filename);

#ifdef _WIN32
        auto pathStr = filepath.toStdWString();
#else
        auto pathStr = filepath.toStdString();
#endif
        SndfileHandle wf(pathStr.c_str(), SFM_WRITE, sndFormat, 1, sr);
        if (!wf) {
            QMessageBox::warning(this, tr("Export Error"), tr("Failed to open file for writing: %1").arg(filepath));
            continue;
        }

        sf_count_t frameCount = end - start;
        sf_count_t written = wf.write(samples.data() + start, frameCount);
        if (written != frameCount) {
            QMessageBox::warning(this, tr("Export Error"), tr("Failed to write all samples to: %1").arg(filepath));
            continue;
        }
        ++exported;
    }

    QMessageBox::information(this, tr("Export Complete"),
                             tr("Exported %1 slice files to:\n%2").arg(exported).arg(outputDir));
}

void SlicerPage::refreshBoundaries() {
    m_boundaryModel->setSlicePoints(m_slicePoints);
    m_container->invalidateBoundaryModel();
}

QMenuBar *SlicerPage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("打开音频文件(&O)..."), this, &SlicerPage::onOpenAudioFiles);
    fileMenu->addAction(QStringLiteral("打开音频目录(&D)..."), this, &SlicerPage::onOpenAudioDirectory);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window())
            w->close();
    });

    auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
    auto *actSlice = processMenu->addAction(QStringLiteral("自动切片"), this, &SlicerPage::onAutoSlice);
    actSlice->setShortcut(QKeySequence(Qt::Key_S));
    auto *actReSlice = processMenu->addAction(QStringLiteral("重新切片"), this, &SlicerPage::onAutoSlice);
    actReSlice->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_S));
    processMenu->addSeparator();
    auto *actImport = processMenu->addAction(QStringLiteral("导入切点..."), this, &SlicerPage::onImportMarkers);
    actImport->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    auto *actSave = processMenu->addAction(QStringLiteral("保存切点..."), this, &SlicerPage::onSaveMarkers);
    actSave->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    processMenu->addSeparator();
    auto *actExport = processMenu->addAction(QStringLiteral("导出切片音频..."), this, &SlicerPage::onExportAudio);
    actExport->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    processMenu->addAction(QStringLiteral("批量导出全部切片..."), this, &SlicerPage::onBatchExportAll);

    return bar;
}

QString SlicerPage::windowTitle() const {
    return QStringLiteral("切片");
}

void SlicerPage::onActivated() {
    static const dstools::SettingsKey<QString> kHSplitterState("Layout/hSplitterState", "");
    static const dstools::SettingsKey<QString> kSplitterState("Layout/containerSplitterState", "");
    static const dstools::SettingsKey<QString> kChartOrder("Layout/chartOrder", "");

    auto hState = m_settings.get(kHSplitterState);
    if (!hState.isEmpty())
        m_hSplitter->restoreState(QByteArray::fromBase64(hState.toUtf8()));

    auto sState = m_settings.get(kSplitterState);
    if (!sState.isEmpty())
        m_container->restoreSplitterState(QByteArray::fromBase64(sState.toUtf8()));

    auto chartOrder = m_settings.get(kChartOrder);
    if (!chartOrder.isEmpty())
        m_container->setChartOrder(chartOrder.split(QLatin1Char(','), Qt::SkipEmptyParts));

    m_container->restoreChartVisibility();
    m_container->restoreResolution();
}

bool SlicerPage::onDeactivating() {
    static const dstools::SettingsKey<QString> kHSplitterState("Layout/hSplitterState", "");
    static const dstools::SettingsKey<QString> kSplitterState("Layout/containerSplitterState", "");
    static const dstools::SettingsKey<QString> kChartOrder("Layout/chartOrder", "");

    if (m_playWidget)
        m_playWidget->setPlaying(false);

    m_settings.set(kHSplitterState, QString::fromLatin1(m_hSplitter->saveState().toBase64()));
    m_settings.set(kSplitterState, QString::fromLatin1(m_container->saveSplitterState().toBase64()));
    m_settings.set(kChartOrder, m_container->chartOrder().join(QLatin1Char(',')));
    m_container->saveChartVisibility();
    m_container->saveResolution();
    return true;
}

void SlicerPage::onOpenAudioFiles() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择音频文件"), {},
        QStringLiteral("音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg *.opus);;所有文件 (*)"));
    if (!files.isEmpty())
        m_audioFileList->addFiles(files);
}

void SlicerPage::onOpenAudioDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择音频目录"));
    if (!dir.isEmpty())
        m_audioFileList->addDirectory(dir);
}

void SlicerPage::updateSlicerListPanel() {
    double totalDuration = 0.0;
    if (m_sampleRate > 0 && !m_samples.empty())
        totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;

    QString prefix = QStringLiteral("slice");
    if (m_audioFileList && !m_audioFileList->currentFilePath().isEmpty())
        prefix = QFileInfo(m_audioFileList->currentFilePath()).completeBaseName();

    m_sliceListPanel->setSliceData(m_slicePoints, totalDuration, prefix);
}

void SlicerPage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && m_playWidget) {
        m_playWidget->setPlaying(!m_playWidget->isPlaying());
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete && m_selectedBoundary >= 0 &&
        m_selectedBoundary < static_cast<int>(m_slicePoints.size())) {
        auto refreshFn = [this]() {
            refreshBoundaries();
            updateSlicerListPanel();
        };
        m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, m_selectedBoundary, refreshFn));
        m_selectedBoundary = -1;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Z && (event->modifiers() & Qt::ControlModifier)) {
        if (event->modifiers() & Qt::ShiftModifier)
            m_undoStack->redo();
        else
            m_undoStack->undo();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void SlicerPage::saveCurrentSlicePoints() {
    if (!m_currentAudioPath.isEmpty()) {
        m_fileSlicePoints[m_currentAudioPath] = m_slicePoints;
    }
}

void SlicerPage::loadSlicePointsForFile(const QString &filePath) {
    auto it = m_fileSlicePoints.find(filePath);
    if (it != m_fileSlicePoints.end()) {
        m_slicePoints = it->second;
    } else {
        m_slicePoints.clear();
    }
}

void SlicerPage::updateFileProgress() {
    int total = m_audioFileList->fileCount();
    int completed = 0;
    for (const auto &[path, points] : m_fileSlicePoints) {
        if (!points.empty())
            ++completed;
    }
    m_audioFileList->progressTracker()->setProgress(completed, total);
}

void SlicerPage::autoSliceFiles(const QStringList &filePaths) {
    AudioUtil::SlicerParams params;
    params.threshold = m_thresholdSpin->value();
    params.minLength = m_minLengthSpin->value();
    params.minInterval = m_minIntervalSpin->value();
    params.hopSize = m_hopSizeSpin->value();
    params.maxSilKept = m_maxSilenceSpin->value();

    for (const QString &filePath : filePaths) {
        if (m_fileSlicePoints.count(filePath) && !m_fileSlicePoints[filePath].empty())
            continue;

        dstools::audio::AudioDecoder decoder;
        if (!decoder.open(filePath))
            continue;

        auto fmt = decoder.format();
        int sr = fmt.sampleRate();
        int channels = fmt.channels();

        std::vector<float> allSamples;
        constexpr int kBufSize = 4096;
        std::vector<float> buffer(kBufSize);
        while (true) {
            int read = decoder.read(buffer.data(), 0, kBufSize);
            if (read <= 0)
                break;
            allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
        }
        decoder.close();

        size_t numFrames = allSamples.size() / channels;
        std::vector<float> mono(numFrames);
        if (channels > 1) {
            for (size_t i = 0; i < numFrames; ++i) {
                float sum = 0.0f;
                for (int c = 0; c < channels; ++c)
                    sum += allSamples[i * channels + c];
                mono[i] = sum / static_cast<float>(channels);
            }
        } else {
            mono.assign(allSamples.begin(), allSamples.end());
        }

        if (mono.empty())
            continue;

        auto slicer = AudioUtil::Slicer::fromMilliseconds(sr, params);
        auto markers = slicer.slice(mono);

        std::vector<double> newPoints;
        for (size_t i = 0; i + 1 < markers.size(); ++i) {
            int64_t boundary = markers[i].second;
            if (markers[i + 1].first > boundary)
                boundary = (boundary + markers[i + 1].first) / 2;
            double cutTime = static_cast<double>(boundary) / sr;
            newPoints.push_back(cutTime);
        }

        m_fileSlicePoints[filePath] = std::move(newPoints);
    }

    if (!m_currentAudioPath.isEmpty() && m_fileSlicePoints.count(m_currentAudioPath)) {
        loadSlicePointsForFile(m_currentAudioPath);
        refreshBoundaries();
        updateSlicerListPanel();
    }

    updateFileProgress();
}

int SlicerPage::performBatchExport(const QString &outputDir, int digits, int sndFormat) {
    int totalExported = 0;

    for (const auto &[audioPath, slicePoints] : m_fileSlicePoints) {
        if (slicePoints.empty())
            continue;

        dstools::audio::AudioDecoder decoder;
        if (!decoder.open(audioPath))
            continue;

        auto fmt = decoder.format();
        int sr = fmt.sampleRate();
        int channels = fmt.channels();

        std::vector<float> allSamples;
        constexpr int kBufSize = 4096;
        std::vector<float> buffer(kBufSize);
        while (true) {
            int read = decoder.read(buffer.data(), 0, kBufSize);
            if (read <= 0)
                break;
            allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
        }
        decoder.close();

        size_t numFrames = allSamples.size() / channels;
        std::vector<float> mono(numFrames);
        if (channels > 1) {
            for (size_t i = 0; i < numFrames; ++i) {
                float sum = 0.0f;
                for (int c = 0; c < channels; ++c)
                    sum += allSamples[i * channels + c];
                mono[i] = sum / static_cast<float>(channels);
            }
        } else {
            mono.assign(allSamples.begin(), allSamples.end());
        }

        QDir dir(outputDir);
        QString prefix = QFileInfo(audioPath).completeBaseName();
        int numSegments = static_cast<int>(slicePoints.size()) + 1;
        for (int i = 0; i < numSegments; ++i) {
            double startSec = (i == 0) ? 0.0 : slicePoints[i - 1];
            double endSec =
                (i < static_cast<int>(slicePoints.size())) ? slicePoints[i] : static_cast<double>(mono.size()) / sr;
            int startSamp = static_cast<int>(startSec * sr);
            int endSamp = std::min(static_cast<int>(endSec * sr), static_cast<int>(mono.size()));
            if (endSamp <= startSamp)
                continue;

            QString filename = QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0'));
            QString filepath = dir.filePath(filename);

#ifdef _WIN32
            auto pathStr = filepath.toStdWString();
#else
            auto pathStr = filepath.toStdString();
#endif
            SndfileHandle wf(pathStr.c_str(), SFM_WRITE, sndFormat, 1, sr);
            if (!wf)
                continue;

            sf_count_t frameCount = endSamp - startSamp;
            wf.write(mono.data() + startSamp, frameCount);
            ++totalExported;
        }
    }

    return totalExported;
}

void SlicerPage::onBatchExportAll() {
    saveCurrentSlicePoints();

    bool hasSlices = false;
    for (const auto &[path, points] : m_fileSlicePoints) {
        if (!points.empty()) {
            hasSlices = true;
            break;
        }
    }
    if (!hasSlices) {
        QMessageBox::information(this, tr("Batch Export"),
                                 tr("No files have been sliced. Slice audio files first."));
        return;
    }

    SliceExportDialog dlg(this);
    dlg.setDefaultPrefix(QStringLiteral("batch"));
    dlg.setWindowTitle(tr("Batch Export All Sliced Audio"));
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString outputDir = dlg.outputDir();
    if (outputDir.isEmpty())
        return;
    QDir dir(outputDir);
    if (!dir.exists())
        dir.mkpath(outputDir);

    int digits = dlg.numDigits();
    int sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    switch (dlg.bitDepth()) {
        case SliceExportDialog::BitDepth::PCM16:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
            break;
        case SliceExportDialog::BitDepth::PCM24:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
            break;
        case SliceExportDialog::BitDepth::PCM32:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
            break;
        case SliceExportDialog::BitDepth::Float32:
            sndFormat = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
            break;
    }

    int totalExported = performBatchExport(outputDir, digits, sndFormat);

    QMessageBox::information(this, tr("Batch Export Complete"),
                             tr("Exported %1 slice files to:\n%2").arg(totalExported).arg(outputDir));
}

void SlicerPage::loadAudioFile(const QString &filePath) {
    dstools::audio::AudioDecoder decoder;
    if (!decoder.open(filePath))
        return;

    auto fmt = decoder.format();
    m_sampleRate = fmt.sampleRate();
    int channels = fmt.channels();

    std::vector<float> allSamples;
    constexpr int kBufSize = 4096;
    std::vector<float> buffer(kBufSize);
    while (true) {
        int read = decoder.read(buffer.data(), 0, kBufSize);
        if (read <= 0) break;
        allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
    }
    decoder.close();

    if (channels > 1) {
        size_t numFrames = allSamples.size() / channels;
        m_samples.resize(numFrames);
        for (size_t i = 0; i < numFrames; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += allSamples[i * channels + c];
            m_samples[i] = sum / static_cast<float>(channels);
        }
    } else {
        m_samples = std::move(allSamples);
    }

    m_playWidget->openFile(filePath);

    double duration = m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;
    m_boundaryModel->setDuration(duration);

    // Set viewport FIRST with actual sample rate, THEN load audio data into widgets.
    // This ensures chart widgets receive correct PPS when setAudioData triggers
    // their initial cache rebuild.
    m_viewport->setAudioParams(m_sampleRate, static_cast<int64_t>(m_samples.size()));
    m_container->fitToWindow();
    {
        double dur = m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;
        DSFW_LOG_INFO("audio", ("Loaded audio: " + filePath.toStdString()
                      + " (" + std::to_string(dur) + "s @ " + std::to_string(m_sampleRate) + "Hz)").c_str());
    }

    m_waveformChart->setAudioData(m_samples, m_sampleRate);
    m_spectrogramChart->setAudioData(m_samples, m_sampleRate);
    m_container->setAudioData(m_samples, m_sampleRate);
}

} // namespace dstools
