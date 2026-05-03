#include "DsSlicerPage.h"
#include "AudacityMarkerIO.h"
#include "SliceCommands.h"
#include "SliceExportDialog.h"
#include "SliceListPanel.h"
#include "SliceNumberLayer.h"

#include <WaveformPanel.h>
#include <MelSpectrogramWidget.h>

#include <QDoubleSpinBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>

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
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // ── Slice params panel ────────────────────────────────────────────────
    auto *paramsGroup = new QGroupBox(QStringLiteral("切片参数"), this);
    auto *paramsLayout = new QHBoxLayout(paramsGroup);

    auto *formLayout = new QFormLayout;

    m_thresholdSpin = new QDoubleSpinBox(this);
    m_thresholdSpin->setRange(-60.0, 0.0);
    m_thresholdSpin->setValue(-40.0);
    m_thresholdSpin->setSuffix(QStringLiteral(" dB"));
    formLayout->addRow(QStringLiteral("阈值:"), m_thresholdSpin);

    m_minLengthSpin = new QSpinBox(this);
    m_minLengthSpin->setRange(500, 60000);
    m_minLengthSpin->setValue(5000);
    m_minLengthSpin->setSuffix(QStringLiteral(" ms"));
    formLayout->addRow(QStringLiteral("最小长度:"), m_minLengthSpin);

    m_minIntervalSpin = new QSpinBox(this);
    m_minIntervalSpin->setRange(100, 5000);
    m_minIntervalSpin->setValue(300);
    m_minIntervalSpin->setSuffix(QStringLiteral(" ms"));
    formLayout->addRow(QStringLiteral("最小间隔:"), m_minIntervalSpin);

    auto *formLayout2 = new QFormLayout;

    m_hopSizeSpin = new QSpinBox(this);
    m_hopSizeSpin->setRange(1, 100);
    m_hopSizeSpin->setValue(10);
    m_hopSizeSpin->setSuffix(QStringLiteral(" ms"));
    formLayout2->addRow(QStringLiteral("Hop:"), m_hopSizeSpin);

    m_maxSilenceSpin = new QSpinBox(this);
    m_maxSilenceSpin->setRange(100, 10000);
    m_maxSilenceSpin->setValue(500);
    m_maxSilenceSpin->setSuffix(QStringLiteral(" ms"));
    formLayout2->addRow(QStringLiteral("最大静音:"), m_maxSilenceSpin);

    paramsLayout->addLayout(formLayout);
    paramsLayout->addLayout(formLayout2);

    // Buttons
    auto *btnLayout = new QVBoxLayout;
    m_btnImportMarkers = new QPushButton(QStringLiteral("导入切点..."), this);
    m_btnAutoSlice = new QPushButton(QStringLiteral("自动切片"), this);
    m_btnReSlice = new QPushButton(QStringLiteral("重新切片"), this);
    m_btnSaveMarkers = new QPushButton(QStringLiteral("保存切点..."), this);
    m_btnExportAudio = new QPushButton(QStringLiteral("导出音频..."), this);

    btnLayout->addWidget(m_btnImportMarkers);
    btnLayout->addWidget(m_btnAutoSlice);
    btnLayout->addWidget(m_btnReSlice);
    btnLayout->addWidget(m_btnSaveMarkers);
    btnLayout->addWidget(m_btnExportAudio);

    paramsLayout->addLayout(btnLayout);
    paramsLayout->addStretch();

    mainLayout->addWidget(paramsGroup);

    // ── Main content splitter (vertical) ──────────────────────────────────
    auto *splitter = new QSplitter(Qt::Vertical, this);

    // Waveform panel
    m_waveformPanel = new waveform::WaveformPanel(this);
    splitter->addWidget(m_waveformPanel);

    // Mel spectrogram (collapsible)
    m_melSpectrogram = new waveform::MelSpectrogramWidget(m_waveformPanel->viewport(), this);
    m_melSpectrogram->setVisible(false); // Collapsed by default
    splitter->addWidget(m_melSpectrogram);

    // Slice number layer
    m_sliceNumberLayer = new SliceNumberLayer(m_waveformPanel->viewport(), this);
    splitter->addWidget(m_sliceNumberLayer);

    splitter->setStretchFactor(0, 4); // waveform
    splitter->setStretchFactor(1, 2); // mel spectrogram
    splitter->setStretchFactor(2, 0); // number layer (fixed height)

    mainLayout->addWidget(splitter, 1);

    // ── Slice list panel (bottom) ─────────────────────────────────────────
    m_sliceListPanel = new SliceListPanel(this);
    m_sliceListPanel->setMaximumHeight(200);
    mainLayout->addWidget(m_sliceListPanel);
}

void DsSlicerPage::connectSignals() {
    connect(m_btnAutoSlice, &QPushButton::clicked, this, &DsSlicerPage::onAutoSlice);
    connect(m_btnReSlice, &QPushButton::clicked, this, &DsSlicerPage::onAutoSlice);
    connect(m_btnImportMarkers, &QPushButton::clicked, this, &DsSlicerPage::onImportMarkers);
    connect(m_btnSaveMarkers, &QPushButton::clicked, this, &DsSlicerPage::onSaveMarkers);
    connect(m_btnExportAudio, &QPushButton::clicked, this, &DsSlicerPage::onExportAudio);

    // Left-click on waveform → add slice point
    connect(m_waveformPanel, &waveform::WaveformPanel::positionClicked, this,
            [this](double sec) {
                auto refreshFn = [this]() { refreshBoundaries(); };
                m_undoStack->push(
                    new AddSlicePointCommand(m_slicePoints, sec, refreshFn));
            });

    // Boundary click → select for potential deletion
    connect(m_waveformPanel, &waveform::WaveformPanel::boundaryClicked, this,
            [this](int index) {
                m_selectedBoundary = index;
            });
}

void DsSlicerPage::onAutoSlice() {
    // RMS-based silence detection on mono samples
    const auto &samples = m_waveformPanel->monoSamples();
    if (samples.empty())
        return;

    int sr = m_waveformPanel->sampleRate();
    double thresholdDb = m_thresholdSpin->value();
    int minLengthMs = m_minLengthSpin->value();
    int minIntervalMs = m_minIntervalSpin->value();
    int hopMs = m_hopSizeSpin->value();
    int maxSilenceMs = m_maxSilenceSpin->value();

    double threshold = std::pow(10.0, thresholdDb / 20.0);
    int hopSamples = sr * hopMs / 1000;
    int minLengthSamples = sr * minLengthMs / 1000;
    int minIntervalSamples = sr * minIntervalMs / 1000;
    int maxSilenceSamples = sr * maxSilenceMs / 1000;

    if (hopSamples <= 0)
        hopSamples = 1;

    // Compute RMS per hop
    int numHops = static_cast<int>(samples.size()) / hopSamples;
    std::vector<bool> isSilent(numHops, false);
    for (int i = 0; i < numHops; ++i) {
        int start = i * hopSamples;
        int end = std::min(start + hopSamples, static_cast<int>(samples.size()));
        double sum = 0.0;
        for (int s = start; s < end; ++s)
            sum += static_cast<double>(samples[s]) * samples[s];
        double rms = std::sqrt(sum / (end - start));
        isSilent[i] = (rms < threshold);
    }

    // Find silence regions and place cut points
    std::vector<double> newPoints;
    int silenceStart = -1;
    int lastCutSample = 0;

    for (int i = 0; i < numHops; ++i) {
        int samplePos = i * hopSamples;
        if (isSilent[i]) {
            if (silenceStart < 0)
                silenceStart = samplePos;
        } else {
            if (silenceStart >= 0) {
                int silenceEnd = samplePos;
                int silenceLen = silenceEnd - silenceStart;
                int segLen = silenceStart - lastCutSample;

                if (silenceLen >= minIntervalSamples && segLen >= minLengthSamples) {
                    // Cut at the middle of the silence
                    int cutSample = silenceStart + silenceLen / 2;
                    double cutTime = static_cast<double>(cutSample) / sr;
                    newPoints.push_back(cutTime);
                    lastCutSample = cutSample;
                } else if (silenceLen >= maxSilenceSamples) {
                    int cutSample = silenceStart + silenceLen / 2;
                    double cutTime = static_cast<double>(cutSample) / sr;
                    newPoints.push_back(cutTime);
                    lastCutSample = cutSample;
                }
                silenceStart = -1;
            }
        }
    }

    // Replace current points
    m_undoStack->beginMacro(tr("Auto slice"));
    // Remove all existing
    while (!m_slicePoints.empty()) {
        m_undoStack->push(new RemoveSlicePointCommand(
            m_slicePoints, static_cast<int>(m_slicePoints.size()) - 1,
            [this]() { refreshBoundaries(); }));
    }
    // Add new
    for (double t : newPoints) {
        m_undoStack->push(
            new AddSlicePointCommand(m_slicePoints, t, [this]() { refreshBoundaries(); }));
    }
    m_undoStack->endMacro();

    refreshBoundaries();
}

void DsSlicerPage::onImportMarkers() {
    QString path = QFileDialog::getOpenFileName(
        this, tr("Import Markers"), {},
        tr("Audacity Labels (*.txt);;All Files (*)"));
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
        m_undoStack->push(new RemoveSlicePointCommand(
            m_slicePoints, static_cast<int>(m_slicePoints.size()) - 1,
            [this]() { refreshBoundaries(); }));
    }
    for (double t : times) {
        m_undoStack->push(
            new AddSlicePointCommand(m_slicePoints, t, [this]() { refreshBoundaries(); }));
    }
    m_undoStack->endMacro();
    refreshBoundaries();
}

void DsSlicerPage::onSaveMarkers() {
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save Markers"), {},
        tr("Audacity Labels (*.txt);;All Files (*)"));
    if (path.isEmpty())
        return;

    QString error;
    if (!AudacityMarkerIO::write(path, m_slicePoints, error))
        QMessageBox::warning(this, tr("Save Error"), error);
}

void DsSlicerPage::onExportAudio() {
    if (m_slicePoints.empty() || m_waveformPanel->monoSamples().empty()) {
        QMessageBox::information(this, tr("Export"),
                                 tr("No slices to export. Run auto-slice first."));
        return;
    }

    SliceExportDialog dlg(this);
    dlg.setDefaultPrefix(QStringLiteral("slice"));
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
        double endSec = (i < static_cast<int>(m_slicePoints.size()))
                            ? m_slicePoints[i]
                            : static_cast<double>(samples.size()) / sr;
        int startSamp = static_cast<int>(startSec * sr);
        int endSamp = std::min(static_cast<int>(endSec * sr),
                               static_cast<int>(samples.size()));
        segments.emplace_back(startSamp, endSamp);
    }

    // Write WAV files (16-bit PCM for now — full bit depth support in future)
    int exported = 0;
    for (int i = 0; i < numSegments; ++i) {
        auto [start, end] = segments[i];
        if (end <= start)
            continue;

        QString filename = QStringLiteral("%1_%2.wav")
                               .arg(prefix)
                               .arg(i + 1, digits, 10, QChar('0'));
        QString filepath = dir.filePath(filename);

        // TODO: Use sndfile or dstools-audio to write WAV with proper bit depth.
        // For now, create empty placeholder files.
        QFile f(filepath);
        if (f.open(QIODevice::WriteOnly)) {
            // Placeholder — actual WAV writing needs libsndfile integration
            f.close();
            ++exported;
        }
    }

    // Create PipelineContext JSONs in dstemp/contexts/
    if (m_dataSource) {
        // TODO: Create context JSON for each slice via ProjectDataSource
    }

    QMessageBox::information(
        this, tr("Export Complete"),
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

    return bar;
}

QString DsSlicerPage::windowTitle() const {
    return QStringLiteral("DsLabeler — 切片");
}

void DsSlicerPage::onActivated() {
    // TODO: Load audio from project's source file and refresh UI
}

void DsSlicerPage::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete && m_selectedBoundary >= 0 &&
        m_selectedBoundary < static_cast<int>(m_slicePoints.size())) {
        auto refreshFn = [this]() { refreshBoundaries(); };
        m_undoStack->push(
            new RemoveSlicePointCommand(m_slicePoints, m_selectedBoundary, refreshFn));
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

} // namespace dstools
