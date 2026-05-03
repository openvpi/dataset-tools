#include "DsSlicerPage.h"
#include "AudioFileListPanel.h"
#include "ProjectDataSource.h"
#include "AudacityMarkerIO.h"
#include "SliceCommands.h"
#include "SliceExportDialog.h"
#include "SlicerListPanel.h"
#include "SliceNumberLayer.h"

#include <dstools/DsProject.h>

#include <dstools/DsItemManager.h>
#include <dstools/DsItemRecord.h>

#include <MelSpectrogramWidget.h>
#include <PlaybackController.h>
#include <WaveformPanel.h>

#include <dsfw/widgets/FileProgressTracker.h>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

#include <audio-util/Slicer.h>

#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QToolBar>
#include <QToolButton>
#include <QKeySequence>

#include <QSplitter>
#include <QVBoxLayout>

#include <sndfile.hh>

#include <algorithm>
#include <cmath>

namespace dstools {

    DsSlicerPage::DsSlicerPage(QWidget *parent) : QWidget(parent) {
        m_undoStack = new QUndoStack(this);
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

        // ── Slice params panel ────────────────────────────────────────────────
        auto *paramsGroup = new QGroupBox(QStringLiteral("切片参数"), contentWidget);
        auto *paramsLayout = new QHBoxLayout(paramsGroup);

        auto *formLayout = new QFormLayout;

        m_thresholdSpin = new QDoubleSpinBox(contentWidget);
        m_thresholdSpin->setRange(-60.0, 0.0);
        m_thresholdSpin->setValue(-40.0);
        m_thresholdSpin->setSuffix(QStringLiteral(" dB"));
        formLayout->addRow(QStringLiteral("阈值:"), m_thresholdSpin);

        m_minLengthSpin = new QSpinBox(contentWidget);
        m_minLengthSpin->setRange(500, 60000);
        m_minLengthSpin->setValue(5000);
        m_minLengthSpin->setSuffix(QStringLiteral(" ms"));
        formLayout->addRow(QStringLiteral("最小长度:"), m_minLengthSpin);

        m_minIntervalSpin = new QSpinBox(contentWidget);
        m_minIntervalSpin->setRange(100, 5000);
        m_minIntervalSpin->setValue(300);
        m_minIntervalSpin->setSuffix(QStringLiteral(" ms"));
        formLayout->addRow(QStringLiteral("最小间隔:"), m_minIntervalSpin);

        auto *formLayout2 = new QFormLayout;

        m_hopSizeSpin = new QSpinBox(contentWidget);
        m_hopSizeSpin->setRange(1, 100);
        m_hopSizeSpin->setValue(10);
        m_hopSizeSpin->setSuffix(QStringLiteral(" ms"));
        formLayout2->addRow(QStringLiteral("Hop:"), m_hopSizeSpin);

        m_maxSilenceSpin = new QSpinBox(contentWidget);
        m_maxSilenceSpin->setRange(100, 10000);
        m_maxSilenceSpin->setValue(500);
        m_maxSilenceSpin->setSuffix(QStringLiteral(" ms"));
        formLayout2->addRow(QStringLiteral("最大静音:"), m_maxSilenceSpin);

        paramsLayout->addLayout(formLayout);
        paramsLayout->addLayout(formLayout2);

        // Buttons
        auto *btnLayout = new QVBoxLayout;
        m_btnImportMarkers = new QPushButton(QStringLiteral("导入切点..."), contentWidget);
        m_btnAutoSlice = new QPushButton(QStringLiteral("自动切片"), contentWidget);
        m_btnReSlice = new QPushButton(QStringLiteral("重新切片"), contentWidget);
        m_btnSaveMarkers = new QPushButton(QStringLiteral("保存切点..."), contentWidget);
        m_btnExportAudio = new QPushButton(QStringLiteral("导出音频..."), contentWidget);

        btnLayout->addWidget(m_btnImportMarkers);
        btnLayout->addWidget(m_btnAutoSlice);
        btnLayout->addWidget(m_btnReSlice);
        btnLayout->addWidget(m_btnSaveMarkers);
        btnLayout->addWidget(m_btnExportAudio);

        paramsLayout->addLayout(btnLayout);
        paramsLayout->addStretch();

        mainLayout->addWidget(paramsGroup);

        // ── Main content splitter (vertical) ──────────────────────────────────
        auto *splitter = new QSplitter(Qt::Vertical, contentWidget);

        // Waveform panel
        m_waveformPanel = new waveform::WaveformPanel(contentWidget);
        splitter->addWidget(m_waveformPanel);

        // Mel spectrogram (collapsible)
        m_melSpectrogram = new waveform::MelSpectrogramWidget(m_waveformPanel->viewport(), contentWidget);
        m_melSpectrogram->setVisible(false); // Collapsed by default
        splitter->addWidget(m_melSpectrogram);

        // Slice number layer
        m_sliceNumberLayer = new SliceNumberLayer(m_waveformPanel->viewport(), contentWidget);
        splitter->addWidget(m_sliceNumberLayer);

        splitter->setStretchFactor(0, 4); // waveform
        splitter->setStretchFactor(1, 2); // mel spectrogram
        splitter->setStretchFactor(2, 0); // number layer (fixed height)

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
            m_waveformPanel->setToolMode(waveform::ToolMode::Pointer);
        });
        connect(m_btnKnife, &QToolButton::clicked, this, [this]() {
            m_btnKnife->setChecked(true);
            m_btnPointer->setChecked(false);
            m_waveformPanel->setToolMode(waveform::ToolMode::Knife);
        });

        // Left sidebar: audio file selection → load audio for slicing
        connect(m_audioFileList, &AudioFileListPanel::fileSelected, this, [this](const QString &filePath) {
            // Save current file's slice points before switching
            saveCurrentSlicePoints();

            m_waveformPanel->loadAudio(filePath);
            m_currentAudioPath = filePath;
            m_undoStack->clear();

            // Restore saved slice points for this file, or start fresh
            loadSlicePointsForFile(filePath);

            refreshBoundaries();
            updateSlicerListPanel();
            updateFileProgress();
        });

        // Update progress when files are added or removed
        connect(m_audioFileList, &AudioFileListPanel::filesAdded, this, [this]() {
            updateFileProgress();
        });
        connect(m_audioFileList, &AudioFileListPanel::filesRemoved, this, [this]() {
            updateFileProgress();
        });

        // Knife mode: left-click on waveform → add slice point
        connect(m_waveformPanel, &waveform::WaveformPanel::positionClicked, this, [this](double sec) {
            auto refreshFn = [this]() { refreshBoundaries(); updateSlicerListPanel(); };
            m_undoStack->push(new AddSlicePointCommand(m_slicePoints, sec, refreshFn));
        });

        // Pointer mode: boundary click → select
        connect(m_waveformPanel, &waveform::WaveformPanel::boundaryClicked, this,
                [this](int index) {
                    m_selectedBoundary = index;
                    m_waveformPanel->setSelectedBoundary(index);
                });

        // Pointer mode: boundary drag → move
        connect(m_waveformPanel, &waveform::WaveformPanel::boundaryMoved, this,
                [this](int index, double oldTime, double newTime) {
                    if (index >= 0 && index < static_cast<int>(m_slicePoints.size())) {
                        auto refreshFn = [this]() { refreshBoundaries(); updateSlicerListPanel(); };
                        m_undoStack->push(new MoveSlicePointCommand(m_slicePoints, index, oldTime, newTime, refreshFn));
                    }
                });

        // Pointer mode: right-click boundary → delete
        connect(m_waveformPanel, &waveform::WaveformPanel::boundaryRightClicked, this,
                [this](int index) {
                    if (index >= 0 && index < static_cast<int>(m_slicePoints.size())) {
                        auto refreshFn = [this]() { refreshBoundaries(); updateSlicerListPanel(); };
                        m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, index, refreshFn));
                        m_selectedBoundary = -1;
                        m_waveformPanel->setSelectedBoundary(-1);
                    }
                });

        // SlicerListPanel context menu: add/delete slice points
        connect(m_sliceListPanel, &SlicerListPanel::sliceDoubleClicked, this,
                [this](int /*index*/, double startSec, double /*endSec*/) {
            m_waveformPanel->scrollToTime(startSec);
        });

        connect(m_sliceListPanel, &SlicerListPanel::addSlicePointRequested, this,
                [this](double timeSec) {
            auto refreshFn = [this]() { refreshBoundaries(); updateSlicerListPanel(); };
            m_undoStack->push(new AddSlicePointCommand(m_slicePoints, timeSec, refreshFn));
        });

        connect(m_sliceListPanel, &SlicerListPanel::removeSlicePointRequested, this,
                [this](int pointIndex) {
            if (pointIndex >= 0 && pointIndex < static_cast<int>(m_slicePoints.size())) {
                auto refreshFn = [this]() { refreshBoundaries(); updateSlicerListPanel(); };
                m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, pointIndex, refreshFn));
            }
        });
    }

    void DsSlicerPage::onAutoSlice() {
        const auto &samples = m_waveformPanel->monoSamples();
        if (samples.empty())
            return;

        int sr = m_waveformPanel->sampleRate();

        // Build slicer params from UI
        AudioUtil::SlicerParams params;
        params.threshold = m_thresholdSpin->value();
        params.minLength = m_minLengthSpin->value();
        params.minInterval = m_minIntervalSpin->value();
        params.hopSize = m_hopSizeSpin->value();
        params.maxSilKept = m_maxSilenceSpin->value();

        auto slicer = AudioUtil::Slicer::fromMilliseconds(sr, params);
        auto markers = slicer.slice(samples);

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
        updateFileProgress();
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
        if (m_slicePoints.empty() || m_waveformPanel->monoSamples().empty()) {
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

        const auto &samples = m_waveformPanel->monoSamples();
        int sr = m_waveformPanel->sampleRate();
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
            case SliceExportDialog::BitDepth::PCM16:  sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16; break;
            case SliceExportDialog::BitDepth::PCM24:  sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_24; break;
            case SliceExportDialog::BitDepth::PCM32:  sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_32; break;
            case SliceExportDialog::BitDepth::Float32: sndFormat = SF_FORMAT_WAV | SF_FORMAT_FLOAT; break;
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
                QMessageBox::warning(this, tr("Export Error"),
                                     tr("Failed to open file for writing: %1").arg(filepath));
                continue;
            }

            sf_count_t frameCount = end - start;
            sf_count_t written = wf.write(samples.data() + start, frameCount);
            if (written != frameCount) {
                QMessageBox::warning(this, tr("Export Error"),
                                     tr("Failed to write all samples to: %1").arg(filepath));
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
                item.audioSource = DsProject::toPosixPath(dir.filePath(
                    QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0'))));
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
                    QString dsitemDir = m_dataSource->workingDir()
                                       + QStringLiteral("/dstemp/dsitems");
                    QDir().mkpath(dsitemDir);

                    DsItemRecord record;
                    record.status = DsItemRecord::Status::Pending;
                    record.inputFile = currentAudioPath.toStdString();
                    record.outputFile = dir.filePath(
                        QStringLiteral("%1_%2.wav").arg(prefix).arg(i + 1, digits, 10, QChar('0')))
                        .toStdString();

                    QT_WARNING_PUSH
                    QT_WARNING_DISABLE_DEPRECATED
                    DsItemManager mgr;
                    QT_WARNING_POP
                    auto dsitemPath = std::filesystem::path(
                        (dsitemDir + QStringLiteral("/") + sliceId + QStringLiteral(".dsitem"))
                            .toStdWString());
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
        std::vector<waveform::BoundaryInfo> boundaries;
        boundaries.reserve(m_slicePoints.size());
        for (double t : m_slicePoints)
            boundaries.push_back({t});
        m_waveformPanel->setBoundaries(boundaries);
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
        const auto &items = project->items();
        if (items.empty())
            return;

        const auto &firstItem = items[0];
        if (!firstItem.audioSource.isEmpty()) {
            QString audioPath = DsProject::fromPosixPath(firstItem.audioSource);
            if (QDir::isRelativePath(audioPath))
                audioPath = QDir(project->workingDirectory()).absoluteFilePath(audioPath);
            if (QFile::exists(audioPath)) {
                m_waveformPanel->loadAudio(audioPath);
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
        const QString dir = QFileDialog::getExistingDirectory(
            this, QStringLiteral("选择音频目录"));
        if (!dir.isEmpty())
            m_audioFileList->addDirectory(dir);
    }

    void DsSlicerPage::updateSlicerListPanel() {
        double totalDuration = 0.0;
        const auto &samples = m_waveformPanel->monoSamples();
        int sr = m_waveformPanel->sampleRate();
        if (sr > 0 && !samples.empty())
            totalDuration = static_cast<double>(samples.size()) / sr;

        QString prefix = QStringLiteral("slice");
        if (m_audioFileList && !m_audioFileList->currentFilePath().isEmpty())
            prefix = QFileInfo(m_audioFileList->currentFilePath()).completeBaseName();

        m_sliceListPanel->setSliceData(m_slicePoints, totalDuration, prefix);
    }

    void DsSlicerPage::keyPressEvent(QKeyEvent *event) {
        if (event->key() == Qt::Key_Delete && m_selectedBoundary >= 0 &&
            m_selectedBoundary < static_cast<int>(m_slicePoints.size())) {
            auto refreshFn = [this]() { refreshBoundaries(); updateSlicerListPanel(); };
            m_undoStack->push(new RemoveSlicePointCommand(m_slicePoints, m_selectedBoundary, refreshFn));
            m_selectedBoundary = -1;
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            // Stop playback
            m_waveformPanel->playback()->stop();
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

    void DsSlicerPage::onBatchExportAll() {
        // Save current file's points first
        saveCurrentSlicePoints();

        // Check that at least one file has slice points
        bool hasSlices = false;
        for (const auto &[path, points] : m_fileSlicePoints) {
            if (!points.empty()) { hasSlices = true; break; }
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
        if (outputDir.isEmpty()) return;
        QDir dir(outputDir);
        if (!dir.exists()) dir.mkpath(outputDir);

        int digits = dlg.numDigits();
        int sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        switch (dlg.bitDepth()) {
            case SliceExportDialog::BitDepth::PCM16:  sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_16; break;
            case SliceExportDialog::BitDepth::PCM24:  sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_24; break;
            case SliceExportDialog::BitDepth::PCM32:  sndFormat = SF_FORMAT_WAV | SF_FORMAT_PCM_32; break;
            case SliceExportDialog::BitDepth::Float32: sndFormat = SF_FORMAT_WAV | SF_FORMAT_FLOAT; break;
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
                if (read <= 0) break;
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
                double endSec = (i < static_cast<int>(slicePoints.size()))
                                    ? slicePoints[i]
                                    : static_cast<double>(mono.size()) / sr;
                int startSamp = static_cast<int>(startSec * sr);
                int endSamp = std::min(static_cast<int>(endSec * sr), static_cast<int>(mono.size()));
                if (endSamp <= startSamp) continue;

                QString filename = QStringLiteral("%1_%2.wav")
                    .arg(prefix).arg(i + 1, digits, 10, QChar('0'));
                QString filepath = dir.filePath(filename);

#ifdef _WIN32
                auto pathStr = filepath.toStdWString();
#else
                auto pathStr = filepath.toStdString();
#endif
                SndfileHandle wf(pathStr.c_str(), SFM_WRITE, sndFormat, 1, sr);
                if (!wf) continue;

                sf_count_t frameCount = endSamp - startSamp;
                wf.write(mono.data() + startSamp, frameCount);
                ++totalExported;
            }
        }

        QMessageBox::information(this, tr("Batch Export Complete"),
            tr("Exported %1 slice files to:\n%2").arg(totalExported).arg(outputDir));
    }

} // namespace dstools
