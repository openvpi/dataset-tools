#include "DsSlicerPage.h"
#include "SliceListPanel.h"
#include "SliceNumberLayer.h"

#include <WaveformPanel.h>
#include <MelSpectrogramWidget.h>

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QVBoxLayout>

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

    // Boundary click on waveform → select slice
    connect(m_waveformPanel, &waveform::WaveformPanel::boundaryClicked, this,
            [this](int index) {
                // TODO: highlight the clicked boundary for editing
                Q_UNUSED(index)
            });
}

void DsSlicerPage::onAutoSlice() {
    // TODO M.5.9: Run RMS slicer with current parameters
}

void DsSlicerPage::onImportMarkers() {
    // TODO M.5.11: Import Audacity marker file
}

void DsSlicerPage::onSaveMarkers() {
    // TODO M.5.11: Save Audacity marker file
}

void DsSlicerPage::onExportAudio() {
    // TODO M.5.10: Show export dialog and slice audio
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

} // namespace dstools
