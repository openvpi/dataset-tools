#include "DsSlicerPage.h"
#include "ProjectDataSource.h"

#include <AudacityMarkerIO.h>
#include <AudioFileListPanel.h>
#include <SliceCommands.h>
#include <SliceExportDialog.h>
#include <SliceNumberLayer.h>
#include <SlicerListPanel.h>

#include <dstools/DsProject.h>

#include <dstools/DsItemManager.h>
#include <dstools/DsItemRecord.h>
#include <dstools/ProjectPaths.h>

#include <ui/WaveformWidget.h>
#include <ui/SpectrogramWidget.h>
#include <ui/TimeRulerWidget.h>
#include <ui/SliceBoundaryModel.h>
#include <QCheckBox>

#include <dsfw/widgets/FileProgressTracker.h>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

#include <audio-util/Slicer.h>

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
#include <QScrollBar>
#include <QToolBar>
#include <QToolButton>

#include <QSplitter>
#include <QVBoxLayout>

#include <sndfile.hh>

#include <algorithm>
#include <cmath>

namespace dstools {

    DsSlicerPage::DsSlicerPage(QWidget *parent) : QWidget(parent) {
        m_undoStack = new QUndoStack(this);
        m_viewport = new dstools::widgets::ViewportController(this);
        m_boundaryModel = new phonemelabeler::SliceBoundaryModel();
        m_playWidget = new dstools::widgets::PlayWidget();
        buildLayout();
        connectSignals();
    }

    DsSlicerPage::~DsSlicerPage() = default;

    void DsSlicerPage::setDataSource(ProjectDataSource *source) {
        m_dataSource = source;
    }

    void DsSlicerPage::buildLayout() {
        // ── Left sidebar: audio file list (supports drag-drop) ────────────────
        m_audioFileList = new AudioFileListPanel(this);
        m_audioFileList->setMinimumWidth(160);
        m_audioFileList->setMaximumWidth(280);
        m_audioFileList->setShowProgress(true);
        m_audioFileList->progressTracker()->setDisplayStyle(dsfw::widgets::FileProgressTracker::ProgressBarStyle);
        m_audioFileList->progressTracker()->setFormat(QStringLiteral("%1 / %2 已标注"));

        // ── Right content area ────────────────────────────────────────────────
        auto *contentWidget = new QWidget(this);
        auto *mainLayout = new QVBoxLayout(contentWidget);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(4);

        // ── Toolbar with tool buttons ─────────────────────────────────────────
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

        // ── Slice params panel (compact single row) ──────────────────────────
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

        // Buttons inline
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

        // ── Main content splitter (vertical) ──────────────────────────────────
        auto *splitter = new QSplitter(Qt::Vertical, contentWidget);

        // Time ruler + Waveform + Scrollbar container
        auto *waveformContainer = new QWidget(contentWidget);
        auto *waveformLayout = new QVBoxLayout(waveformContainer);
        waveformLayout->setContentsMargins(0, 0, 0, 0);
        waveformLayout->setSpacing(0);

        m_timeRuler = new phonemelabeler::TimeRulerWidget(m_viewport, waveformContainer);
        waveformLayout->addWidget(m_timeRuler);

        m_waveformWidget = new phonemelabeler::WaveformWidget(m_viewport, waveformContainer);
        m_waveformWidget->setBoundaryModel(m_boundaryModel);
        m_waveformWidget->setPlayWidget(m_playWidget);
        // No undo stack — slicer handles undo externally via SliceCommands
        waveformLayout->addWidget(m_waveformWidget, 1);

        m_hScrollBar = new QScrollBar(Qt::Horizontal, waveformContainer);
        waveformLayout->addWidget(m_hScrollBar);

        splitter->addWidget(waveformContainer);

        // Spectrogram (collapsible)
        m_spectrogramWidget = new phonemelabeler::SpectrogramWidget(m_viewport, contentWidget);
        m_spectrogramWidget->setBoundaryModel(m_boundaryModel);
        m_spectrogramWidget->setPlayWidget(m_playWidget);
        m_spectrogramWidget->setVisible(true);
        splitter->addWidget(m_spectrogramWidget);

        // Slice number layer
        m_sliceNumberLayer = new SliceNumberLayer(m_viewport, contentWidget);
        splitter->addWidget(m_sliceNumberLayer);

        splitter->setStretchFactor(0, 2); // waveform
        splitter->setStretchFactor(1, 5); // spectrogram
        splitter->setStretchFactor(2, 3); // number layer

        mainLayout->addWidget(splitter, 1);

        // ── Slice list panel (bottom) ─────────────────────────────────────────
        m_sliceListPanel = new SlicerListPanel(contentWidget);
        m_sliceListPanel->setMaximumHeight(200);
        mainLayout->addWidget(m_sliceListPanel);

        // ── Horizontal splitter: sidebar | content ────────────────────────────
        auto *hSplitter = new QSplitter(Qt::Horizontal, this);
        hSplitter->addWidget(m_audioFileList);
        hSplitter->addWidget(contentWidget);
        hSplitter->setStretchFactor(0, 0);
        hSplitter->setStretchFactor(1, 1);

        auto *topLayout = new QVBoxLayout(this);
        topLayout->setContentsMargins(0, 0, 0, 0);
        topLayout->addWidget(hSplitter);
    }

    void DsSlicerPage::connectSignals() {
        connect(m_btnAutoSlice, &QPushButton::clicked, this, &DsSlicerPage::onAutoSlice);
        connect(m_btnReSlice, &QPushButton::clicked, this, &DsSlicerPage::onAutoSlice);
        connect(m_btnImportMarkers, &QPushButton::clicked, this, &DsSlicerPage::onImportMarkers);
        connect(m_btnSaveMarkers, &QPushButton::clicked, this, &DsSlicerPage::onSaveMarkers);
        connect(m_btnExportAudio, &QPushButton::clicked, this, &DsSlicerPage::onExportAudio);

        // ── Tool mode switching ───────────────────────────────────────────────
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

        // Viewport → sync ruler and scrollbar
        connect(m_viewport, &dstools::widgets::ViewportController::viewportChanged,
                m_timeRuler, &phonemelabeler::TimeRulerWidget::setViewport);
        connect(m_viewport, &dstools::widgets::ViewportController::viewportChanged,
                m_waveformWidget, &phonemelabeler::WaveformWidget::setViewport);
        connect(m_viewport, &dstools::widgets::ViewportController::viewportChanged,
                m_spectrogramWidget, &phonemelabeler::SpectrogramWidget::setViewport);
        connect(m_viewport, &dstools::widgets::ViewportController::viewportChanged,
                m_sliceNumberLayer, &SliceNumberLayer::setViewport);
        connect(m_viewport, &dstools::widgets::ViewportController::viewportChanged,
                this, [this](const dstools::widgets::ViewportState &) { updateScrollBar(); });

        // Scrollbar
        connect(m_hScrollBar, &QScrollBar::valueChanged, this, [this](int value) {
            double totalDuration = m_viewport->totalDuration();
            if (totalDuration <= 0.0) return;
            double startSec = value / 1000.0;
            double viewDuration = m_viewport->state().endSec - m_viewport->state().startSec;
            m_viewport->setViewRange(startSec, startSec + viewDuration);
        });

        // Left sidebar: audio file selection → load audio for slicing
        connect(m_audioFileList, &AudioFileListPanel::fileSelected, this, [this](const QString &filePath) {
            saveCurrentSlicePoints();
            loadAudioFile(filePath);
            m_currentAudioPath = filePath;
            m_undoStack->clear();
            loadSlicePointsForFile(filePath);

            // Auto-slice if no existing slice points for this file
            if (m_slicePoints.empty() && !m_samples.empty()) {
                onAutoSlice();
            }

            refreshBoundaries();
            updateSlicerListPanel();
            updateFileProgress();
        });

        // Update progress and auto-slice when files are added
        connect(m_audioFileList, &AudioFileListPanel::filesAdded, this,
                [this](const QStringList &paths) { autoSliceFiles(paths); });
        connect(m_audioFileList, &AudioFileListPanel::filesRemoved, this, [this]() { updateFileProgress(); });

        // Knife mode: left-click on waveform → add slice point
        connect(m_waveformWidget, &phonemelabeler::WaveformWidget::positionClicked, this, [this](double sec) {
            if (m_toolMode == ToolMode::Knife) {
                auto refreshFn = [this]() {
                    refreshBoundaries();
                    updateSlicerListPanel();
                };
                m_undoStack->push(new AddSlicePointCommand(m_slicePoints, sec, refreshFn));
            }
        });

        // Boundary drag finished → create undo command for the move
        connect(m_waveformWidget, &phonemelabeler::WaveformWidget::boundaryDragFinished,
                this, [this](int /*tierIndex*/, int boundaryIndex, dstools::TimePos newTime) {
            if (boundaryIndex >= 0 && boundaryIndex < static_cast<int>(m_slicePoints.size())) {
                // The model was already updated by the widget (no-undo-stack path).
                // Sync m_slicePoints from model.
                m_slicePoints = m_boundaryModel->slicePointsSec();
                refreshBoundaries();
                updateSlicerListPanel();
            }
        });

        // SlicerListPanel context menu: add/delete slice points
        connect(m_sliceListPanel, &SlicerListPanel::sliceDoubleClicked, this,
                [this](int /*index*/, double startSec, double /*endSec*/) {
                    // Scroll viewport to show this time
                    double viewDuration = m_viewport->state().endSec - m_viewport->state().startSec;
                    m_viewport->setViewRange(startSec, startSec + viewDuration);
                });

        connect(m_sliceListPanel, &SlicerListPanel::addSlicePointRequested, this, [this](double timeSec) {
            auto refreshFn = [this]() {
                refreshBoundaries();
                updateSlicerListPanel();
            };
            m_undoStack->push(new AddSlicePointCommand(m_slicePoints, timeSec, refreshFn));
        });

        connect(m_sliceListPanel, &SlicerListPanel::removeSlicePointRequested, this, [this](int pointIndex) {
            if (pointIndex >= 0 && pointIndex < static_cast<int>(m_slicePoints.size())) {
                auto refreshFn = [this]() {
                    refreshBoundaries();
                    updateSlicerListPanel();
                };
                m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, pointIndex, refreshFn));
            }
        });
    }

    void DsSlicerPage::onAutoSlice() {
        if (m_samples.empty())
            return;

        int sr = m_sampleRate;

        // Build slicer params from UI
        AudioUtil::SlicerParams params;
        params.threshold = m_thresholdSpin->value();
        params.minLength = m_minLengthSpin->value();
        params.minInterval = m_minIntervalSpin->value();
        params.hopSize = m_hopSizeSpin->value();
        params.maxSilKept = m_maxSilenceSpin->value();

        auto slicer = AudioUtil::Slicer::fromMilliseconds(sr, params);
        auto markers = slicer.slice(m_samples);

        // Convert marker boundaries to slice points (seconds)
        // markers are (startSample, endSample) pairs; slice points are between segments
        std::vector<double> newPoints;
        for (size_t i = 0; i + 1 < markers.size(); ++i) {
            // Boundary between segment i and i+1: end of segment i
            int64_t boundary = markers[i].second;
            // Average with start of next segment for smoother cut point
            if (markers[i + 1].first > boundary)
                boundary = (boundary + markers[i + 1].first) / 2;
            double cutTime = static_cast<double>(boundary) / sr;
            newPoints.push_back(cutTime);
        }

        // Replace current points
        m_undoStack->beginMacro(tr("Auto slice"));
        // Remove all existing
        while (!m_slicePoints.empty()) {
            m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, static_cast<int>(m_slicePoints.size()) - 1,
                                                          [this]() { refreshBoundaries(); }));
        }
        // Add new
        for (double t : newPoints) {
            m_undoStack->push(new AddSlicePointCommand(m_slicePoints, t, [this]() { refreshBoundaries(); }));
        }
        m_undoStack->endMacro();

        refreshBoundaries();
        updateSlicerListPanel();
        saveCurrentSlicePoints();
        saveSlicerParamsToProject();
        updateFileProgress();
        promptSliceUpdateIfNeeded();
    }

    void DsSlicerPage::onImportMarkers() {
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
        promptSliceUpdateIfNeeded();
    }

    void DsSlicerPage::onSaveMarkers() {
        QString path =
            QFileDialog::getSaveFileName(this, tr("Save Markers"), {}, tr("Audacity Labels (*.txt);;All Files (*)"));
        if (path.isEmpty())
            return;

        QString error;
        if (!AudacityMarkerIO::write(path, m_slicePoints, error))
            QMessageBox::warning(this, tr("Save Error"), error);
    }

    void DsSlicerPage::onExportAudio() {
        if (m_slicePoints.empty() || m_samples.empty()) {
            QMessageBox::information(this, tr("Export"), tr("No slices to export. Run auto-slice first."));
            return;
        }

        SliceExportDialog dlg(this);
        // Default prefix = original audio file's basename (without extension)
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

        // Build segment boundaries
        std::vector<std::pair<int, int>> segments; // (startSample, endSample)
        int numSegments = static_cast<int>(m_slicePoints.size()) + 1;
        for (int i = 0; i < numSegments; ++i) {
            double startSec = (i == 0) ? 0.0 : m_slicePoints[i - 1];
            double endSec = (i < static_cast<int>(m_slicePoints.size())) ? m_slicePoints[i]
                                                                         : static_cast<double>(samples.size()) / sr;
            int startSamp = static_cast<int>(startSec * sr);
            int endSamp = std::min(static_cast<int>(endSec * sr), static_cast<int>(samples.size()));
            segments.emplace_back(startSamp, endSamp);
        }

        // Write WAV files using libsndfile
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
        int channels = (dlg.channelMode() == SliceExportDialog::ChannelExportMode::Mono) ? 1 : 1;

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
            SndfileHandle wf(pathStr.c_str(), SFM_WRITE, sndFormat, channels, sr);
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

        // Create PipelineContext JSONs and register slices as project items
        if (m_dataSource && m_dataSource->project()) {
            auto *project = m_dataSource->project();
            const auto discarded = m_sliceListPanel->discardedIndices();

            // Build items from exported slices — each slice becomes a project item
            std::vector<Item> items;
            for (int i = 0; i < numSegments; ++i) {
                auto [start, end] = segments[i];
                if (end <= start)
                    continue;

                QString sliceId = QStringLiteral("%1_%2").arg(prefix).arg(i + 1, digits, 10, QChar('0'));
                bool isDiscarded = std::find(discarded.begin(), discarded.end(), i) != discarded.end();

                double startSec = static_cast<double>(start) / sr;
                double endSec = static_cast<double>(end) / sr;

                Slice slice;
                slice.id = sliceId;
                slice.name = sliceId;
                slice.inPos = static_cast<int64_t>(startSec * 1000000.0);
                slice.outPos = static_cast<int64_t>(endSec * 1000000.0);
                slice.status = isDiscarded ? QStringLiteral("discarded") : QStringLiteral("active");

                Item item;
                item.id = sliceId;
                item.name = sliceId;
                item.audioSource = DsProject::toPosixPath(
                    dir.filePath(QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0'))));
                item.slices.push_back(std::move(slice));
                items.push_back(std::move(item));

                // Create PipelineContext
                auto *ctx = m_dataSource->context(sliceId);
                if (ctx) {
                    ctx->itemId = sliceId;
                    ctx->audioPath = item.audioSource;
                    m_dataSource->saveContext(sliceId);
                }

                // Create .dsitem record file for pipeline tracking
                {
                    QString dsitemDir = ProjectPaths::dsItemsDir(m_dataSource->workingDir());
                    QDir().mkpath(dsitemDir);

                    DsItemRecord record;
                    record.status = DsItemRecord::Status::Pending;
                    record.inputFile = currentAudioPath.toStdString();
                    record.outputFile =
                        dir.filePath(QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0')))
                            .toStdString();

                    QT_WARNING_PUSH
                    QT_WARNING_DISABLE_DEPRECATED
                    DsItemManager mgr;
                    QT_WARNING_POP
                    auto dsitemPath = std::filesystem::path(
                        (dsitemDir + QStringLiteral("/") + sliceId + QStringLiteral(".dsitem")).toStdWString());
                    mgr.save(record, dsitemPath);
                }
            }

            project->setItems(std::move(items));

            // Save the project
            QString saveError;
            project->save(saveError);

            // Notify data source to refresh slice list
            emit m_dataSource->sliceListChanged();
        }

        QMessageBox::information(this, tr("Export Complete"),
                                 tr("Exported %1 slice files to:\n%2").arg(exported).arg(outputDir));
    }

    void DsSlicerPage::refreshBoundaries() {
        m_boundaryModel->setSlicePoints(m_slicePoints);
        m_waveformWidget->updateBoundaryOverlay();
        m_spectrogramWidget->updateBoundaryOverlay();
        m_sliceNumberLayer->setSlicePoints(m_slicePoints);
    }

    QMenuBar *DsSlicerPage::createMenuBar(QWidget *parent) {
        auto *bar = new QMenuBar(parent);

        auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
        fileMenu->addAction(QStringLiteral("打开音频文件(&O)..."), this, &DsSlicerPage::onOpenAudioFiles);
        fileMenu->addAction(QStringLiteral("打开音频目录(&D)..."), this, &DsSlicerPage::onOpenAudioDirectory);
        fileMenu->addSeparator();
        fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
            if (auto *w = window())
                w->close();
        });

        auto *processMenu = bar->addMenu(QStringLiteral("处理(&P)"));
        processMenu->addAction(QStringLiteral("自动切片"), this, &DsSlicerPage::onAutoSlice);
        processMenu->addAction(QStringLiteral("重新切片"), this, &DsSlicerPage::onAutoSlice);
        processMenu->addSeparator();
        processMenu->addAction(QStringLiteral("导入切点..."), this, &DsSlicerPage::onImportMarkers);
        processMenu->addAction(QStringLiteral("保存切点..."), this, &DsSlicerPage::onSaveMarkers);
        processMenu->addSeparator();
        processMenu->addAction(QStringLiteral("导出切片音频..."), this, &DsSlicerPage::onExportAudio);
        processMenu->addAction(QStringLiteral("批量导出全部切片..."), this, &DsSlicerPage::onBatchExportAll);

        return bar;
    }

    QString DsSlicerPage::windowTitle() const {
        return QStringLiteral("DsLabeler — 切片");
    }

    void DsSlicerPage::onActivated() {
        if (!m_dataSource || !m_dataSource->project())
            return;

        auto *project = m_dataSource->project();

        // Load slicer params from project defaults
        const auto &slicerConfig = project->defaults().slicerConfig;
        m_thresholdSpin->setValue(slicerConfig.threshold);
        m_minLengthSpin->setValue(slicerConfig.minLength);
        m_minIntervalSpin->setValue(slicerConfig.minInterval);
        m_hopSizeSpin->setValue(slicerConfig.hopSize);
        m_maxSilenceSpin->setValue(slicerConfig.maxSilence);

        const auto &items = project->items();
        if (items.empty())
            return;

        const auto &firstItem = items[0];
        if (!firstItem.audioSource.isEmpty()) {
            QString audioPath = DsProject::fromPosixPath(firstItem.audioSource);
            if (QDir::isRelativePath(audioPath))
                audioPath = QDir(project->workingDirectory()).absoluteFilePath(audioPath);
            if (QFile::exists(audioPath)) {
                loadAudioFile(audioPath);
                m_slicePoints.clear();
                refreshBoundaries();
                updateSlicerListPanel();
            }
        }
    }

    void DsSlicerPage::onOpenAudioFiles() {
        const QStringList files = QFileDialog::getOpenFileNames(
            this, QStringLiteral("选择音频文件"), {},
            QStringLiteral("音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg *.opus);;所有文件 (*)"));
        if (!files.isEmpty())
            m_audioFileList->addFiles(files);
    }

    void DsSlicerPage::onOpenAudioDirectory() {
        const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("选择音频目录"));
        if (!dir.isEmpty())
            m_audioFileList->addDirectory(dir);
    }

    void DsSlicerPage::updateSlicerListPanel() {
        double totalDuration = 0.0;
        if (m_sampleRate > 0 && !m_samples.empty())
            totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;

        QString prefix = QStringLiteral("slice");
        if (m_audioFileList && !m_audioFileList->currentFilePath().isEmpty())
            prefix = QFileInfo(m_audioFileList->currentFilePath()).completeBaseName();

        m_sliceListPanel->setSliceData(m_slicePoints, totalDuration, prefix);
    }

    void DsSlicerPage::keyPressEvent(QKeyEvent *event) {
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
            // Stop playback (no-op if no playback controller)
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

    void DsSlicerPage::saveCurrentSlicePoints() {
        if (!m_currentAudioPath.isEmpty()) {
            m_fileSlicePoints[m_currentAudioPath] = m_slicePoints;
        }
    }

    void DsSlicerPage::loadSlicePointsForFile(const QString &filePath) {
        auto it = m_fileSlicePoints.find(filePath);
        if (it != m_fileSlicePoints.end()) {
            m_slicePoints = it->second;
        } else {
            m_slicePoints.clear();
        }
    }

    void DsSlicerPage::updateFileProgress() {
        int total = m_audioFileList->fileCount();
        int completed = 0;
        for (const auto &[path, points] : m_fileSlicePoints) {
            if (!points.empty())
                ++completed;
        }
        m_audioFileList->progressTracker()->setProgress(completed, total);
    }

    void DsSlicerPage::autoSliceFiles(const QStringList &filePaths) {
        AudioUtil::SlicerParams params;
        params.threshold = m_thresholdSpin->value();
        params.minLength = m_minLengthSpin->value();
        params.minInterval = m_minIntervalSpin->value();
        params.hopSize = m_hopSizeSpin->value();
        params.maxSilKept = m_maxSilenceSpin->value();

        for (const QString &filePath : filePaths) {
            // Skip files that already have slice points
            if (m_fileSlicePoints.count(filePath) && !m_fileSlicePoints[filePath].empty())
                continue;

            // Load audio
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

            // Mix to mono
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

            // Run slicer
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

        // If current file was just sliced, refresh display
        if (!m_currentAudioPath.isEmpty() && m_fileSlicePoints.count(m_currentAudioPath)) {
            loadSlicePointsForFile(m_currentAudioPath);
            refreshBoundaries();
            updateSlicerListPanel();
        }

        updateFileProgress();
    }

    void DsSlicerPage::onBatchExportAll() {
        // Save current file's points first
        saveCurrentSlicePoints();

        // Check that at least one file has slice points
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

        int totalExported = 0;

        for (const auto &[audioPath, slicePoints] : m_fileSlicePoints) {
            if (slicePoints.empty())
                continue;

            // Load audio for this file
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

            // Mix to mono
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

            // Build segments
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

        QMessageBox::information(this, tr("Batch Export Complete"),
                                 tr("Exported %1 slice files to:\n%2").arg(totalExported).arg(outputDir));
    }

    void DsSlicerPage::promptSliceUpdateIfNeeded() {
        if (!m_dataSource || !m_dataSource->project())
            return;

        const auto &items = m_dataSource->project()->items();
        if (items.empty())
            return; // No exported items yet, nothing to update

        // Find audio files that have changed slice points and have existing items
        QStringList affectedFiles;
        for (const auto &[filePath, points] : m_fileSlicePoints) {
            QString baseName = QFileInfo(filePath).completeBaseName();
            // Check if any project items came from this audio file
            for (const auto &item : items) {
                if (item.id.startsWith(baseName)) {
                    affectedFiles.append(filePath);
                    break;
                }
            }
        }

        if (affectedFiles.isEmpty())
            return;

        // Build dialog with checkboxes
        QDialog dlg(this);
        dlg.setWindowTitle(QStringLiteral("切点已更新"));
        auto *dlgLayout = new QVBoxLayout(&dlg);

        auto *warnLabel =
            new QLabel(QStringLiteral("⚠ 以下音频文件的切点已更改，但已有导出的切片和标注数据。\n"
                                      "重新切片将移除旧的切片和 dsitem，<b>已标注的歌词、音素、音高数据将丢失</b>。\n\n"
                                      "选择需要重新切片的音频："),
                       &dlg);
        warnLabel->setWordWrap(true);
        dlgLayout->addWidget(warnLabel);

        QList<QCheckBox *> checkboxes;
        for (const QString &file : affectedFiles) {
            auto *cb = new QCheckBox(QFileInfo(file).fileName(), &dlg);
            cb->setChecked(false);
            cb->setProperty("filePath", file);
            checkboxes.append(cb);
            dlgLayout->addWidget(cb);
        }

        auto *btnLayout = new QHBoxLayout;
        auto *btnSkip = new QPushButton(QStringLiteral("跳过"), &dlg);
        auto *btnApply = new QPushButton(QStringLiteral("重新切片选中"), &dlg);
        btnLayout->addStretch();
        btnLayout->addWidget(btnSkip);
        btnLayout->addWidget(btnApply);
        dlgLayout->addLayout(btnLayout);

        connect(btnSkip, &QPushButton::clicked, &dlg, &QDialog::reject);
        connect(btnApply, &QPushButton::clicked, &dlg, &QDialog::accept);

        if (dlg.exec() != QDialog::Accepted)
            return;

        // Remove old items for selected files and notify
        auto *project = m_dataSource->project();
        auto currentItems = project->items();

        for (auto *cb : checkboxes) {
            if (!cb->isChecked())
                continue;

            QString filePath = cb->property("filePath").toString();
            QString baseName = QFileInfo(filePath).completeBaseName();

            // Remove items matching this base name
            currentItems.erase(std::remove_if(currentItems.begin(), currentItems.end(),
                                              [&](const Item &item) { return item.id.startsWith(baseName); }),
                               currentItems.end());

            // Remove dsitem files
            if (m_dataSource) {
                QString dsitemDir = ProjectPaths::dsItemsDir(m_dataSource->workingDir());
                QDir dir(dsitemDir);
                QStringList dsitemFiles = dir.entryList({baseName + QStringLiteral("*.dsitem")}, QDir::Files);
                for (const QString &f : dsitemFiles)
                    QFile::remove(dir.absoluteFilePath(f));
            }
        }

        project->setItems(std::move(currentItems));
        QString saveError;
        project->save(saveError);
        emit m_dataSource->sliceListChanged();
    }

    void DsSlicerPage::saveSlicerParamsToProject() {
        if (!m_dataSource || !m_dataSource->project())
            return;

        auto defaults = m_dataSource->project()->defaults();
        defaults.slicerConfig.threshold = m_thresholdSpin->value();
        defaults.slicerConfig.minLength = m_minLengthSpin->value();
        defaults.slicerConfig.minInterval = m_minIntervalSpin->value();
        defaults.slicerConfig.hopSize = m_hopSizeSpin->value();
        defaults.slicerConfig.maxSilence = m_maxSilenceSpin->value();
        m_dataSource->project()->setDefaults(defaults);

        QString saveError;
        m_dataSource->project()->save(saveError);
    }

    void DsSlicerPage::loadAudioFile(const QString &filePath) {
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

        // Mix to mono
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

        // Feed to visualization widgets
        m_waveformWidget->setAudioData(m_samples, m_sampleRate);
        m_spectrogramWidget->setAudioData(m_samples, m_sampleRate);

        // Open audio for playback
        m_playWidget->openFile(filePath);

        // Update boundary model duration
        double duration = m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;
        m_boundaryModel->setDuration(duration);
        m_viewport->setTotalDuration(duration);
        m_viewport->setViewRange(0.0, duration);
        updateScrollBar();
    }

    void DsSlicerPage::updateScrollBar() {
        double totalDuration = m_viewport->totalDuration();
        if (totalDuration <= 0.0) {
            m_hScrollBar->setRange(0, 0);
            return;
        }
        double viewDuration = m_viewport->state().endSec - m_viewport->state().startSec;
        int maxVal = static_cast<int>((totalDuration - viewDuration) * 1000.0);
        m_hScrollBar->setRange(0, qMax(0, maxVal));
        m_hScrollBar->setValue(static_cast<int>(m_viewport->state().startSec * 1000.0));
        m_hScrollBar->setPageStep(static_cast<int>(viewDuration * 1000.0));
    }

} // namespace dstools
