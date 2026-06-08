#include "EditorContainerBase.h"

#include "AudioVisualizerContainer.h"
#include <dsfw/widgets/ViewportController.h>
#include <dsfw/widgets/PlayWidget.h>
#include <WaveformChartPanel.h>
#include <SpectrogramChartPanel.h>
#include <PowerChartPanel.h>
#include <MouthCurveChartPanel.h>

#include <QAction>
#include <QDataStream>
#include <QIODevice>
#include <QSplitter>
#include <QToolBar>
#include <QToolButton>

namespace dstools {


EditorContainerBase::EditorContainerBase(const QString &settingsGroup, QWidget *parent)
    : AudioEditorWidgetBase(parent), m_settingsGroup(settingsGroup) {
    m_playWidget = new dsfw::widgets::PlayWidget(this);
    m_playWidget->hide();
}

EditorContainerBase::~dsfw::EditorContainerBase() = default;

void EditorContainerBase::setupContainerAndPlayWidget(int defaultResolution) {
    m_defaultResolution = defaultResolution;
    m_container = new dsfw::AudioVisualizerContainer(m_settingsGroup, this);
    m_container->setDefaultResolution(defaultResolution);
    m_viewport = m_container->viewport();
    m_container->setPlayWidget(m_playWidget);
}

void EditorContainerBase::setupSidebarToggle(QSplitter *mainSplitter, QToolBar *toolbar) {
    m_mainSplitter = mainSplitter;
    m_sidebarExpandedSize = m_mainSplitter->sizes();

    static const QString kToggleStyle = QStringLiteral(
        "QToolButton { padding: 3px 8px; border-radius: 3px; }"
        "QToolButton:hover { background-color: #2A2A38; color: #A0A0B0; }"
        "QToolButton:pressed { background-color: #1E1E2E; }"
        "QToolButton:checked {"
        "  background-color: #3d6fa5;"
        "  color: #ffffff;"
        "  border: 1px solid #5a9fd4;"
        "}"
        "QToolButton:!checked {"
        "  background-color: transparent;"
        "  color: #888888;"
        "  border: 1px solid #555555;"
        "}"
    );

    if (toolbar) {
        auto *spacer = new QWidget(toolbar);
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        toolbar->addWidget(spacer);
        m_actToggleSidebar = new QAction(QStringLiteral("\u25C0"), this);
        m_actToggleSidebar->setToolTip(tr("折叠/展开右侧面板"));
        m_actToggleSidebar->setCheckable(true);
        toolbar->addAction(m_actToggleSidebar);
        if (auto *btn = qobject_cast<QToolButton *>(toolbar->widgetForAction(m_actToggleSidebar)))
            btn->setStyleSheet(kToggleStyle);
    }

    connect(m_actToggleSidebar, &QAction::toggled, this, [this](bool checked) {
        if (!m_mainSplitter) return;
        if (checked) {
            m_sidebarExpandedSize = m_mainSplitter->sizes();
            m_mainSplitter->setSizes({1, 0});
        } else {
            if (!m_sidebarExpandedSize.isEmpty())
                m_mainSplitter->setSizes(m_sidebarExpandedSize);
            else
                m_mainSplitter->setSizes({3, 1});
        }
    });
}

QByteArray EditorContainerBase::saveSplitterState() const {
    QByteArray result;
    QDataStream ds(&result, QIODevice::WriteOnly);
    ds << (m_mainSplitter ? m_mainSplitter->saveState() : QByteArray());
    ds << (m_container ? m_container->saveSplitterState() : QByteArray());
    ds << (m_actToggleSidebar ? m_actToggleSidebar->isChecked() : false);
    return result;
}

void EditorContainerBase::restoreSplitterState(const QByteArray &state) {
    if (state.isEmpty())
        return;
    QDataStream ds(state);
    QByteArray mainState, containerState;
    bool sidebarCollapsed = false;
    ds >> mainState >> containerState;
    if (!ds.atEnd())
        ds >> sidebarCollapsed;
    if (!mainState.isEmpty() && m_mainSplitter)
        m_mainSplitter->restoreState(mainState);
    if (!containerState.isEmpty() && m_container)
        m_container->restoreSplitterState(containerState);
    if (m_actToggleSidebar) {
        m_actToggleSidebar->setChecked(sidebarCollapsed);
        if (sidebarCollapsed && m_mainSplitter) {
            m_mainSplitter->setSizes({1, 0});
        }
    }
}

void EditorContainerBase::saveViewportResolution() const {
    if (m_container)
        m_container->saveResolution();
}

void EditorContainerBase::restoreViewportResolution() const {
    if (m_container)
        m_container->restoreResolution();
}

void EditorContainerBase::setViewportResolutionKey(const QString &key) {
    if (m_container)
        m_container->setResolutionKey(key);
}

void EditorContainerBase::addWaveformChart(int tierIndex, int panelIndex, double stretch) {
    dstools::WaveformChartPanel::registerChartConfig();
    m_waveformChart = new dstools::WaveformChartPanel(m_viewport, m_container);
    m_waveformChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("waveform"), m_waveformChart, tierIndex, panelIndex, stretch);
}

void EditorContainerBase::addSpectrogramChart(int tierIndex, int panelIndex, double stretch) {
    dstools::SpectrogramChartPanel::registerChartConfig();
    m_spectrogramChart = new dstools::SpectrogramChartPanel(m_viewport, m_container);
    m_spectrogramChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("spectrogram"), m_spectrogramChart, tierIndex, panelIndex, stretch);
}

void EditorContainerBase::addPowerChart(int tierIndex, int panelIndex, double stretch) {
    dstools::PowerChartPanel::registerChartConfig();
    m_powerChart = new dstools::PowerChartPanel(m_viewport, m_container);
    m_powerChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("power"), m_powerChart, tierIndex, panelIndex, stretch);
}

void EditorContainerBase::addMouthCurveChart(int tierIndex, int panelIndex, double stretch) {
    MouthCurveChartPanel::registerChartConfig();
    m_mouthCurveChart = new dsfw::MouthCurveChartPanel(m_viewport, m_container);
    m_mouthCurveChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("mouthCurve"), m_mouthCurveChart, tierIndex, panelIndex, stretch);
}

void EditorContainerBase::setAllChartsBoundaryModel(dstools::IBoundaryModel *model) {
    if (m_waveformChart)
        m_waveformChart->setBoundaryModel(model);
    if (m_spectrogramChart)
        m_spectrogramChart->setBoundaryModel(model);
    if (m_powerChart)
        m_powerChart->setBoundaryModel(model);
}

void EditorContainerBase::setMouthCurveData(const MouthCurve &curve) {
    if (m_mouthCurveChart)
        m_mouthCurveChart->setData(curve);
}

void EditorContainerBase::onZoomIn() {
    m_viewport->zoomIn(m_viewport->viewCenter());
    m_container->adjustViewRangeToResolution();
    emit zoomChanged(m_viewport->resolution());
}

void EditorContainerBase::onZoomOut() {
    m_viewport->zoomOut(m_viewport->viewCenter());
    m_container->adjustViewRangeToResolution();
    emit zoomChanged(m_viewport->resolution());
}

void EditorContainerBase::onZoomReset() {
    m_viewport->setResolution(m_defaultResolution);
    m_container->updateViewRangeFromResolution();
    emit zoomChanged(m_viewport->resolution());
}

void EditorContainerBase::updateZoomActions(QAction *in, QAction *out, QAction *reset) {
    if (in)
        in->setEnabled(m_viewport->canZoomIn());
    if (out)
        out->setEnabled(m_viewport->canZoomOut());
    Q_UNUSED(reset)
}

void EditorContainerBase::createPlaybackActions() {
    m_actPlayPause = new QAction(this);
    m_actPlayPause->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
    m_actPlayPause->setStatusTip(tr("Play/Pause (Space)"));

    m_actStop = new QAction(this);
    m_actStop->setIcon(QIcon(QStringLiteral(":/icons/stop.svg")));
    m_actStop->setStatusTip(tr("Stop (Escape)"));

    connect(m_actPlayPause, &QAction::triggered, this, &EditorContainerBase::onPlayPause);
    connect(m_actStop, &QAction::triggered, this, &EditorContainerBase::onStop);

    connect(m_playWidget, &dsfw::widgets::PlayWidget::playRequested,
            this, &EditorContainerBase::updatePlayPauseIcon);
    connect(m_playWidget, &dsfw::widgets::PlayWidget::playStopped,
            this, &EditorContainerBase::updatePlayPauseIcon);
}

void EditorContainerBase::onPlayPause() {
    if (!m_playWidget) return;
    m_playWidget->setPlaying(!m_playWidget->isPlaying());
}

void EditorContainerBase::onStop() {
    if (!m_playWidget) return;
    m_playWidget->setPlaying(false);
}

void EditorContainerBase::updatePlayPauseIcon() {
    if (!m_actPlayPause || !m_playWidget) return;
    if (m_playWidget->isPlaying()) {
        m_actPlayPause->setIcon(QIcon(QStringLiteral(":/icons/pause.svg")));
    } else {
        m_actPlayPause->setIcon(QIcon(QStringLiteral(":/icons/play.svg")));
    }
}

} // namespace dstools
