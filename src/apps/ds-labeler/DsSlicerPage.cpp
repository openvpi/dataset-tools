#include "DsSlicerPage.h"

#include "AudioFileListPanel.h"
#include "ProjectDataSource.h"

#include <AudacityMarkerIO.h>
#include <SliceCommands.h>
#include <SliceExportDialog.h>
#include <SliceListPanel.h>

#include <dstools/DsProject.h>
#include <dstools/ProjectPaths.h>

#include <dsfw/JsonHelper.h>
#include <dsfw/widgets/FileProgressTracker.h>

#include <dstools/AudioDecoder.h>

#include <audio-util/Slicer.h>

#include <QCheckBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <sndfile.hh>

#include <algorithm>
#include <cmath>

namespace dstools {

DsSlicerPage::DsSlicerPage(QWidget *parent) : SlicerPage(parent) {
    connectProjectSignals();
}

DsSlicerPage::~DsSlicerPage() = default;

void DsSlicerPage::setDataSource(ProjectDataSource *source) {
    m_dataSource = source;
}

void DsSlicerPage::connectProjectSignals() {
    QObject::disconnect(m_btnExportAudio, SIGNAL(clicked()), this, SLOT(onExportAudio()));
    connect(m_btnExportAudio, &QPushButton::clicked, this, &DsSlicerPage::onExportMenu);

    connect(m_audioFileList, &AudioFileListPanel::filesAdded, this, [this](const QStringList &) {
        saveSlicerStateToProject();
    });
    connect(m_audioFileList, &AudioFileListPanel::filesRemoved, this, [this]() {
        saveSlicerStateToProject();
    });
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
    for (const auto &[path, points] : m_fileSlicePoints) {
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
    auto *btnCurrent = msgBox.addButton(tr("Current File"), QMessageBox::AcceptRole);
    auto *btnAll = msgBox.addButton(tr("All Files (%1)").arg(slicedCount), QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();
    auto *clicked = msgBox.clickedButton();
    if (clicked == btnCurrent)
        onExportAudio();
    else if (clicked == btnAll)
        onBatchExportAll();
}

void DsSlicerPage::onExportAudio() {
    SlicerPage::onExportAudio();

    if (!m_dataSource || !m_dataSource->project())
        return;

    auto *project = m_dataSource->project();
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

        auto *ctx = m_dataSource->context(sliceId);
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

            auto j = dsCtx.toJson();
            auto dsitemPath = std::filesystem::path(
                (dsitemDir + QStringLiteral("/") + sliceId + QStringLiteral(".dsitem")).toStdWString());
            JsonHelper::saveFile(dsitemPath, j);
        }
    }

    project->setItems(std::move(items));
    QString saveError;
    project->save(saveError);
    emit m_dataSource->sliceListChanged();
}

void DsSlicerPage::onBatchExportAll() {
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

    if (!m_dataSource || !m_dataSource->project())
        return;

    auto *project = m_dataSource->project();
    auto currentItems = project->items();

    for (const auto &[audioPath, slicePoints] : m_fileSlicePoints) {
        if (slicePoints.empty())
            continue;

        int sr = 0;
        {
            dstools::audio::AudioDecoder decoder;
            if (!decoder.open(audioPath))
                continue;
            sr = decoder.format().sampleRate();
        }

        QString prefix = QFileInfo(audioPath).completeBaseName();
        int numSegments = static_cast<int>(slicePoints.size()) + 1;
        for (int i = 0; i < numSegments; ++i) {
            QString sliceId = QStringLiteral("%1_%2").arg(prefix).arg(i + 1, digits, 10, QChar('0'));

            bool exists = false;
            for (const auto &item : currentItems) {
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
    QString saveError;
    project->save(saveError);
    emit m_dataSource->sliceListChanged();
}

void DsSlicerPage::promptSliceUpdateIfNeeded() {
    if (!m_dataSource || !m_dataSource->project())
        return;

    const auto &items = m_dataSource->project()->items();
    if (items.empty())
        return;

    QStringList affectedFiles;
    for (const auto &[filePath, points] : m_fileSlicePoints) {
        QString baseName = QFileInfo(filePath).completeBaseName();
        for (const auto &item : items) {
            if (item.id.startsWith(baseName)) {
                affectedFiles.append(filePath);
                break;
            }
        }
    }

    if (affectedFiles.isEmpty())
        return;

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

    auto *project = m_dataSource->project();
    auto currentItems = project->items();

    for (auto *cb : checkboxes) {
        if (!cb->isChecked())
            continue;

        QString filePath = cb->property("filePath").toString();
        QString baseName = QFileInfo(filePath).completeBaseName();

        currentItems.erase(std::remove_if(currentItems.begin(), currentItems.end(),
                                          [&](const Item &item) { return item.id.startsWith(baseName); }),
                           currentItems.end());

        QString dsitemDir = ProjectPaths::dsItemsDir(m_dataSource->workingDir());
        QDir dir(dsitemDir);
        QStringList dsitemFiles = dir.entryList({baseName + QStringLiteral("*.dsitem")}, QDir::Files);
        for (const QString &f : dsitemFiles)
            QFile::remove(dir.absoluteFilePath(f));
    }

    project->setItems(std::move(currentItems));
    QString saveError;
    project->save(saveError);
    emit m_dataSource->sliceListChanged();
}

void DsSlicerPage::saveSlicerParamsToProject() {
    if (!m_dataSource || !m_dataSource->project())
        return;

    auto *project = m_dataSource->project();
    auto state = project->slicerState();
    state.params.threshold = m_thresholdSpin->value();
    state.params.minLength = m_minLengthSpin->value();
    state.params.minInterval = m_minIntervalSpin->value();
    state.params.hopSize = m_hopSizeSpin->value();
    state.params.maxSilence = m_maxSilenceSpin->value();
    project->setSlicerState(std::move(state));

    QString saveError;
    project->save(saveError);
}

void DsSlicerPage::saveSlicerStateToProject() {
    if (!m_dataSource || !m_dataSource->project())
        return;

    auto *project = m_dataSource->project();
    auto state = project->slicerState();
    state.audioFiles = m_audioFileList->filePaths();
    state.slicePoints = m_fileSlicePoints;
    project->setSlicerState(std::move(state));

    QString saveError;
    project->save(saveError);
}

QMenuBar *DsSlicerPage::createMenuBar(QWidget *parent) {
    auto *bar = SlicerPage::createMenuBar(parent);

    auto *fileMenu = bar->findChild<QMenu *>();
    if (!fileMenu) {
        for (auto *action : bar->actions()) {
            if (auto *menu = action->menu()) {
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

    auto *project = m_dataSource->project();

    const auto &slicerParams = project->slicerState().params;
    m_thresholdSpin->setValue(slicerParams.threshold);
    m_minLengthSpin->setValue(slicerParams.minLength);
    m_minIntervalSpin->setValue(slicerParams.minInterval);
    m_hopSizeSpin->setValue(slicerParams.hopSize);
    m_maxSilenceSpin->setValue(slicerParams.maxSilence);

    const auto &slicerState = project->slicerState();

    if (!slicerState.audioFiles.isEmpty() && m_audioFileList->fileCount() == 0) {
        QStringList resolvedPaths;
        for (const auto &relPath : slicerState.audioFiles) {
            QString nativePath = relPath;
            if (QDir::isRelativePath(nativePath))
                nativePath = QDir(project->workingDirectory()).absoluteFilePath(nativePath);
            if (QFile::exists(nativePath))
                resolvedPaths.append(nativePath);
        }
        if (!resolvedPaths.isEmpty())
            m_audioFileList->addFiles(resolvedPaths);
    }

    for (const auto &[relPath, points] : slicerState.slicePoints) {
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

    const auto &items = project->items();
    if (items.empty())
        return;

    const auto &firstItem = items[0];
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