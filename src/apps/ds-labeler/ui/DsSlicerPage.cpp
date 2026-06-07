#include "DsSlicerPage.h"

#include "AudioFileListPanel.h"
#include "core/ProjectDataSource.h"
#include "SlicerIntegrityGuard.h"

#include <SliceCommands.h>
#include <SliceExportDialog.h>
#include <SliceListPanel.h>

#include <dstools/DsProject.h>
#include <dstools/ProjectPaths.h>

#include <dsfw/JsonHelper.h>
#include <dsfw/widgets/FileProgressTracker.h>
#include <dsfw/widgets/ProgressDialog.h>

#include <dsfw/audio/AudioPipeline.h>

#include <dsfw/signal/Slicer.h>

#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QVBoxLayout>

#include <QtConcurrent>

#include <algorithm>
#include <cmath>

namespace dstools {

using namespace dsfw;

DsSlicerPage::DsSlicerPage(QWidget* parent) : SlicerPage(parent) {
    connectProjectSignals();
}

DsSlicerPage::~DsSlicerPage() = default;

void DsSlicerPage::setDataSource(ProjectDataSource* source) {
    m_dataSource = source;
}

void DsSlicerPage::connectProjectSignals() {
    QObject::disconnect(m_btnExportAudio, &QPushButton::clicked, this, &DsSlicerPage::onExportAudio);
    connect(m_btnExportAudio, &QPushButton::clicked, this, &DsSlicerPage::onExportMenu);

    connect(m_audioFileList, &AudioFileListPanel::filesAdded, this,
            [this](const QStringList&) { saveSlicerStateToProject(); });
    connect(m_audioFileList, &AudioFileListPanel::filesRemoved, this, [this]() { saveSlicerStateToProject(); });
}

void DsSlicerPage::onAutoSlice() {
    SlicerPage::onAutoSlice();
    saveSlicerParamsToProject();
    saveSlicerStateToProject();
    promptSliceUpdateIfNeeded();
}

void DsSlicerPage::onImportMarkers() {
    SlicerPage::onImportMarkers();
    saveSlicerStateToProject();
    promptSliceUpdateIfNeeded();
}

void DsSlicerPage::saveCurrentSlicePoints() {
    SlicerPage::saveCurrentSlicePoints();
    saveSlicerStateToProject();
}

void DsSlicerPage::onExportMenu() {
    saveCurrentSlicePoints();
    int slicedCount = 0;
    for (const auto& [path, points] : m_fileSlicePoints) {
        if (!points.empty())
            ++slicedCount;
    }

    if (slicedCount == 0 && m_slicePoints.empty()) {
        QMessageBox::information(this, tr("Export"), tr("No slices to export. Run auto-slice first."));
        return;
    }

    if (slicedCount <= 1) {
        onExportAudio();
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("Export Sliced Audio"));
    msgBox.setText(tr("Choose export scope:"));
    auto* btnCurrent = msgBox.addButton(tr("Current File"), QMessageBox::AcceptRole);
    auto* btnAll = msgBox.addButton(tr("All Files (%1)").arg(slicedCount), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();
    auto* clicked = msgBox.clickedButton();
    if (clicked == btnCurrent)
        onExportAudio();
    else if (clicked == btnAll)
        onBatchExportAll();
}

void DsSlicerPage::onExportAudio() {
    SlicerPage::onExportAudio();

    if (!m_dataSource || !m_dataSource->project())
        return;

    auto* project = m_dataSource->project();
    const auto discarded = m_sliceListPanel->discardedIndices();

    int sr = m_sampleRate;
    int numSegments = static_cast<int>(m_slicePoints.size()) + 1;
    QString prefix;
    if (m_audioFileList && !m_audioFileList->currentFilePath().isEmpty())
        prefix = QFileInfo(m_audioFileList->currentFilePath()).completeBaseName();
    int digits = 3;

    std::vector<Item> items;
    for (int i = 0; i < numSegments; ++i) {
        double startSec = (i == 0) ? 0.0 : m_slicePoints[i - 1];
        double endSec = (i < static_cast<int>(m_slicePoints.size())) ? m_slicePoints[i]
                                                                     : static_cast<double>(m_samples.size()) / sr;
        int startSamp = static_cast<int>(startSec * sr);
        int endSamp = std::min(static_cast<int>(endSec * sr), static_cast<int>(m_samples.size()));
        if (endSamp <= startSamp)
            continue;

        QString sliceId = QStringLiteral("%1_%2").arg(prefix).arg(i + 1, digits, 10, QChar('0'));
        bool isDiscarded = std::find(discarded.begin(), discarded.end(), i) != discarded.end();

        Slice slice;
        slice.id = sliceId;
        slice.name = sliceId;
        slice.inPos = static_cast<int64_t>(startSec * 1000000.0);
        slice.outPos = static_cast<int64_t>(endSec * 1000000.0);
        slice.status = isDiscarded ? QStringLiteral("discarded") : QStringLiteral("active");

        Item item;
        item.id = sliceId;
        item.name = sliceId;
        item.audioSource = QStringLiteral("dstemp/slices/%1.wav").arg(sliceId);
        item.slices.push_back(std::move(slice));
        items.push_back(std::move(item));

        auto* ctx = m_dataSource->context(sliceId);
        if (ctx) {
            ctx->itemId = sliceId;
            ctx->audioPath = item.audioSource;
            m_dataSource->saveContext(sliceId);
        }

        {
            QString dsitemDir = ProjectPaths::dsItemsDir(m_dataSource->workingDir());
            QDir().mkpath(dsitemDir);

            PipelineContext dsCtx;
            dsCtx.itemId = sliceId;
            dsCtx.audioPath = item.audioSource;
            dsCtx.status = PipelineContext::Status::Active;

            auto j = nlohmann::json::parse(dsCtx.toJsonString());
            auto dsitemPath =
                std::filesystem::path(QDir(dsitemDir).filePath(sliceId + QStringLiteral(".dsitem")).toStdWString());
            JsonHelper::saveFile(dsitemPath, j);
        }
    }

    project->setItems(std::move(items));
    project->saveFile();
    emit m_dataSource->sliceListChanged();

    SlicerIntegrityGuard guard;
    guard.recordExportedSlicePoints(m_dataSource->workingDir(), m_fileSlicePoints);
}

void DsSlicerPage::onBatchExportAll() {
    saveCurrentSlicePoints();

    bool hasSlices = false;
    for (const auto& [path, points] : m_fileSlicePoints) {
        if (!points.empty()) {
            hasSlices = true;
            break;
        }
    }
    if (!hasSlices) {
        QMessageBox::information(this, tr("Batch Export"), tr("No files have been sliced. Slice audio files first."));
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
    auto bitDepth = dlg.bitDepth();
    auto sndFormat = bitDepth == SliceExportDialog::BitDepth::Float32 ? dsfw::audio::SampleFormat::Float32
                   : bitDepth == SliceExportDialog::BitDepth::PCM32   ? dsfw::audio::SampleFormat::Int32
                   :                                                     dsfw::audio::SampleFormat::Int16;

    int totalFiles = 0;
    for (const auto& [path, points] : m_fileSlicePoints) {
        if (!points.empty())
            ++totalFiles;
    }

    auto* progressDlg = new dsfw::widgets::ProgressDialog(tr("Batch Export"), this);
    progressDlg->setRange(0, totalFiles);
    progressDlg->setLabelText(tr("Exporting %1 files...").arg(totalFiles));
    progressDlg->setCancelButtonEnabled(false);

    QPointer<DsSlicerPage> guard(this);
    auto* watcher = new QFutureWatcher<int>(this);
    connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher, progressDlg, outputDir, digits, guard]() {
        if (!guard)
            return;
        int totalExported = watcher->result();

        if (m_dataSource && m_dataSource->project()) {
            auto* project = m_dataSource->project();
            auto currentItems = project->items();
            for (const auto& [audioPath, slicePoints] : m_fileSlicePoints) {
                if (slicePoints.empty())
                    continue;

                int sr = 0;
                {
                    auto pipeline = dsfw::audio::AudioPipeline::create();
                    auto probeResult = pipeline.probe(audioPath.toStdString());
                    if (!probeResult.ok())
                        continue;
                    sr = probeResult.value().sampleRate;
                }

                QString prefix = QFileInfo(audioPath).completeBaseName();
                int numSegments = static_cast<int>(slicePoints.size()) + 1;
                for (int i = 0; i < numSegments; ++i) {
                    QString sliceId = QStringLiteral("%1_%2").arg(prefix).arg(i + 1, digits, 10, QChar('0'));
                    bool exists = false;
                    for (const auto& item : currentItems) {
                        if (item.id == sliceId) {
                            exists = true;
                            break;
                        }
                    }
                    if (exists)
                        continue;

                    double startSec = (i == 0) ? 0.0 : slicePoints[i - 1];
                    double endSec = (i < static_cast<int>(slicePoints.size())) ? slicePoints[i] : 0.0;
                    if (endSec <= startSec)
                        continue;

                    Slice slice;
                    slice.id = sliceId;
                    slice.name = sliceId;
                    slice.inPos = static_cast<int64_t>(startSec * 1000000.0);
                    slice.outPos = static_cast<int64_t>(endSec * 1000000.0);
                    slice.status = QStringLiteral("active");

                    Item item;
                    item.id = sliceId;
                    item.name = sliceId;
                    item.audioSource = QStringLiteral("dstemp/slices/%1.wav").arg(sliceId);
                    item.slices.push_back(std::move(slice));
                    currentItems.push_back(std::move(item));
                }
            }
            project->setItems(std::move(currentItems));
            project->saveFile();
            emit m_dataSource->sliceListChanged();

            SlicerIntegrityGuard slicerGuard;
            slicerGuard.recordExportedSlicePoints(m_dataSource->workingDir(), m_fileSlicePoints);
        }

        progressDlg->close();
        progressDlg->deleteLater();
        watcher->deleteLater();
        QMessageBox::information(guard, tr("Batch Export Complete"),
                                 tr("Exported %1 slice files to:\n%2").arg(totalExported).arg(outputDir));
    });

    QPointer<DsSlicerPage> self(this);

    auto future = QtConcurrent::run([self, outputDir, digits, sndFormat]() {
        if (!self)
            return -1;
        return self->performBatchExport(outputDir, digits, sndFormat);
    });
    watcher->setFuture(future);

    progressDlg->exec();
}

void DsSlicerPage::promptSliceUpdateIfNeeded() {
    if (!m_dataSource || !m_dataSource->project())
        return;

    const auto& items = m_dataSource->project()->items();
    if (items.empty())
        return;

    QStringList affectedFiles;
    std::vector<QString> affectedBaseNames;
    for (const auto& [filePath, points] : m_fileSlicePoints) {
        QString baseName = QFileInfo(filePath).completeBaseName();
        for (const auto& item : items) {
            if (item.id.startsWith(baseName)) {
                affectedFiles.append(filePath);
                affectedBaseNames.push_back(baseName);
                break;
            }
        }
    }

    if (affectedFiles.isEmpty())
        return;

    SlicerIntegrityGuard guard;
    auto reports = guard.scanAffectedSlices(m_dataSource->workingDir(), affectedBaseNames);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("切点已更新"));
    auto* dlgLayout = new QVBoxLayout(&dlg);

    auto* warnLabel = new QLabel(QStringLiteral("以下音频文件的切点已更改，但已有导出的切片和标注数据。\n"
                                                "重新切片将移除旧的切片和 dsitem，<b>已标注数据将丢失</b>。\n\n"
                                                "选择需要重新切片的音频："),
                                 &dlg);
    warnLabel->setWordWrap(true);
    dlgLayout->addWidget(warnLabel);

    QHash<QString, FileIntegrityReport*> reportByBaseName;
    for (auto& report : reports) {
        reportByBaseName[report.baseName] = &report;
    }

    QList<QCheckBox*> checkboxes;
    for (const QString& file : affectedFiles) {
        QString baseName = QFileInfo(file).completeBaseName();
        auto* cb = new QCheckBox(QFileInfo(file).fileName(), &dlg);
        cb->setChecked(false);
        cb->setProperty("filePath", file);
        cb->setProperty("baseName", baseName);
        checkboxes.append(cb);

        auto* fileLayout = new QVBoxLayout;
        fileLayout->addWidget(cb);

        if (auto* report = reportByBaseName.value(baseName)) {
            QString summary = guard.dataLossSummary(*report);
            if (report->hasAnnotatedData()) {
                auto* summaryLabel = new QLabel(QStringLiteral("    ↳ %1").arg(summary), &dlg);
                summaryLabel->setStyleSheet(QStringLiteral("color: #e07030; font-size: 11px; margin-left: 20px;"));
                fileLayout->addWidget(summaryLabel);
            }
        }

        dlgLayout->addLayout(fileLayout);
    }

    auto* btnLayout = new QHBoxLayout;
    auto* btnSkip = new QPushButton(QStringLiteral("跳过"), &dlg);
    auto* btnApply = new QPushButton(QStringLiteral("重新切片选中"), &dlg);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSkip);
    btnLayout->addWidget(btnApply);
    dlgLayout->addLayout(btnLayout);

    connect(btnSkip, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(btnApply, &QPushButton::clicked, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted)
        return;

    auto* project = m_dataSource->project();
    auto currentItems = project->items();
    const QString workingDir = m_dataSource->workingDir();

    for (auto* cb : checkboxes) {
        if (!cb->isChecked())
            continue;

        QString filePath = cb->property("filePath").toString();
        QString baseName = cb->property("baseName").toString();

        if (auto* report = reportByBaseName.value(baseName)) {
            for (auto& slice : report->slices) {
                QString backupPath;
                guard.backupSlice(slice.dstextPath, backupPath);

                QString dsitemPath =
                    QDir(ProjectPaths::dsItemsDir(workingDir)).filePath(slice.sliceId + QStringLiteral(".dsitem"));
                if (QFile::exists(dsitemPath)) {
                    QFile::copy(dsitemPath,
                                SlicerIntegrityGuard::makeBackupPath(dsitemPath, QDateTime::currentDateTime()));
                }
            }
        }

        currentItems.erase(std::remove_if(currentItems.begin(), currentItems.end(),
                                          [&](const Item& item) { return item.id.startsWith(baseName); }),
                           currentItems.end());

        QString dsitemDir = ProjectPaths::dsItemsDir(workingDir);
        QDir dir(dsitemDir);
        QStringList dsitemFiles = dir.entryList({baseName + QStringLiteral("*.dsitem")}, QDir::Files);
        for (const QString& f : dsitemFiles)
            QFile::remove(dir.absoluteFilePath(f));
    }

    project->setItems(std::move(currentItems));
    project->saveFile();
    emit m_dataSource->sliceListChanged();
}

void DsSlicerPage::saveSlicerParamsToProject() {
    if (!m_dataSource || !m_dataSource->project())
        return;

    auto* project = m_dataSource->project();
    auto state = project->slicerState();
    state.params.threshold = m_thresholdSpin->value();
    state.params.minLength = m_minLengthSpin->value();
    state.params.minInterval = m_minIntervalSpin->value();
    state.params.hopSize = m_hopSizeSpin->value();
    state.params.maxSilence = m_maxSilenceSpin->value();
    project->setSlicerState(std::move(state));

    project->saveFile();
}

void DsSlicerPage::saveSlicerStateToProject() {
    if (!m_dataSource || !m_dataSource->project())
        return;

    auto* project = m_dataSource->project();
    auto state = project->slicerState();
    state.audioFiles = m_audioFileList->filePaths();
    state.slicePoints = m_fileSlicePoints;
    project->setSlicerState(std::move(state));

    project->saveFile();
}

QMenuBar* DsSlicerPage::createMenuBar(QWidget* parent) {
    auto* bar = SlicerPage::createMenuBar(parent);

    auto* fileMenu = bar->findChild<QMenu*>();
    if (!fileMenu) {
        for (auto* action : bar->actions()) {
            if (auto* menu = action->menu()) {
                fileMenu = menu;
                break;
            }
        }
    }

    return bar;
}

QString DsSlicerPage::windowTitle() const {
    return QStringLiteral("DsLabeler — 切片");
}

void DsSlicerPage::onActivated() {
    SlicerPage::onActivated();

    if (!m_dataSource || !m_dataSource->project())
        return;

    auto* project = m_dataSource->project();

    const auto& slicerParams = project->slicerState().params;
    m_thresholdSpin->setValue(slicerParams.threshold);
    m_minLengthSpin->setValue(slicerParams.minLength);
    m_minIntervalSpin->setValue(slicerParams.minInterval);
    m_hopSizeSpin->setValue(slicerParams.hopSize);
    m_maxSilenceSpin->setValue(slicerParams.maxSilence);

    const auto& slicerState = project->slicerState();

    if (!slicerState.audioFiles.isEmpty() && m_audioFileList->fileCount() == 0) {
        QStringList resolvedPaths;
        for (const auto& relPath : slicerState.audioFiles) {
            QString nativePath = relPath;
            if (QDir::isRelativePath(nativePath))
                nativePath = QDir(project->workingDirectory()).absoluteFilePath(nativePath);
            if (QFile::exists(nativePath))
                resolvedPaths.append(nativePath);
        }
        if (!resolvedPaths.isEmpty())
            m_audioFileList->addFiles(resolvedPaths);
    }

    for (const auto& [relPath, points] : slicerState.slicePoints) {
        QString nativePath = relPath;
        if (QDir::isRelativePath(nativePath))
            nativePath = QDir(project->workingDirectory()).absoluteFilePath(nativePath);
        if (!points.empty())
            m_fileSlicePoints[nativePath] = points;
    }

    if (!slicerState.audioFiles.isEmpty()) {
        QString firstPath = slicerState.audioFiles.first();
        if (QDir::isRelativePath(firstPath))
            firstPath = QDir(project->workingDirectory()).absoluteFilePath(firstPath);
        if (QFile::exists(firstPath)) {
            loadAudioFile(firstPath);
            m_currentAudioPath = firstPath;
            loadSlicePointsForFile(firstPath);
            refreshBoundaries();
            updateSlicerListPanel();
            updateFileProgress();
            return;
        }
    }

    const auto& items = project->items();
    if (items.empty())
        return;

    const auto& firstItem = items[0];
    if (!firstItem.audioSource.isEmpty()) {
        QString audioPath = firstItem.audioSource;
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

bool DsSlicerPage::onDeactivating() {
    if (m_playWidget)
        m_playWidget->setPlaying(false);
    return SlicerPage::onDeactivating();
}

} // namespace dstools