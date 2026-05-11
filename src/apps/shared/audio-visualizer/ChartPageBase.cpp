#include "ChartPageBase.h"

#include "AudioVisualizerContainer.h"
#include <dstools/ViewportController.h>
#include <dstools/PlayWidget.h>
#include <ui/WaveformChartPanel.h>
#include <ui/SpectrogramChartPanel.h>
#include <ui/PowerChartPanel.h>

#include <QAction>

namespace dstools {

ChartPageBase::ChartPageBase(const QString &settingsGroup, QWidget *parent)
    : QWidget(parent), m_settingsGroup(settingsGroup) {
    m_playWidget = new dstools::widgets::PlayWidget(this);
    m_playWidget->hide();
}

ChartPageBase::~ChartPageBase() = default;

void ChartPageBase::setupContainerAndPlayWidget(int defaultResolution) {
    m_defaultResolution = defaultResolution;
    m_container = new AudioVisualizerContainer(m_settingsGroup, this);
    m_container->setDefaultResolution(defaultResolution);
    m_viewport = m_container->viewport();
    m_container->setPlayWidget(m_playWidget);
}

void ChartPageBase::addWaveformChart(int tierIndex, int panelIndex, double stretch) {
    m_waveformChart = new phonemelabeler::WaveformChartPanel(m_viewport, m_container);
    m_waveformChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("waveform"), m_waveformChart, tierIndex, panelIndex, stretch);
}

void ChartPageBase::addSpectrogramChart(int tierIndex, int panelIndex, double stretch) {
    m_spectrogramChart = new phonemelabeler::SpectrogramChartPanel(m_viewport, m_container);
    m_spectrogramChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("spectrogram"), m_spectrogramChart, tierIndex, panelIndex, stretch);
}

void ChartPageBase::addPowerChart(int tierIndex, int panelIndex, double stretch) {
    m_powerChart = new phonemelabeler::PowerChartPanel(m_viewport, m_container);
    m_powerChart->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("power"), m_powerChart, tierIndex, panelIndex, stretch);
}

void ChartPageBase::setAllChartsBoundaryModel(phonemelabeler::IBoundaryModel *model) {
    if (m_waveformChart)
        m_waveformChart->setBoundaryModel(model);
    if (m_spectrogramChart)
        m_spectrogramChart->setBoundaryModel(model);
    if (m_powerChart)
        m_powerChart->setBoundaryModel(model);
}

void ChartPageBase::onZoomIn() {
    m_viewport->zoomIn(m_viewport->viewCenter());
    m_container->adjustViewRangeToResolution();
    emit zoomChanged(m_viewport->resolution());
}

void ChartPageBase::onZoomOut() {
    m_viewport->zoomOut(m_viewport->viewCenter());
    m_container->adjustViewRangeToResolution();
    emit zoomChanged(m_viewport->resolution());
}

void ChartPageBase::onZoomReset() {
    m_viewport->setResolution(m_defaultResolution);
    m_container->updateViewRangeFromResolution();
    emit zoomChanged(m_viewport->resolution());
}

void ChartPageBase::updateZoomActions(QAction *in, QAction *out, QAction *reset) {
    if (in)
        in->setEnabled(m_viewport->canZoomIn());
    if (out)
        out->setEnabled(m_viewport->canZoomOut());
    Q_UNUSED(reset)
}

} // namespace dstools