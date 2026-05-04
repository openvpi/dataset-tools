#include "AudioVisualizerContainer.h"
#include "MiniMapScrollBar.h"
#include "TierLabelArea.h"

#include <ui/BoundaryOverlayWidget.h>
#include <ui/IBoundaryModel.h>
#include <ui/TimeRulerWidget.h>

#include <dsfw/widgets/PlayWidget.h>

namespace dstools {

using dsfw::widgets::ViewportController;
using dsfw::widgets::ViewportState;
using dsfw::widgets::PlayWidget;

AudioVisualizerContainer::AudioVisualizerContainer(const QString &settingsAppName,
                                                   QWidget *parent)
    : QWidget(parent), m_settings(settingsAppName) {
    m_viewport = new ViewportController(this);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_miniMap = new MiniMapScrollBar(m_viewport, this);
    mainLayout->addWidget(m_miniMap);

    m_timeRuler = new TimeRulerWidget(m_viewport, this);
    mainLayout->addWidget(m_timeRuler);

    m_tierLabelArea = new TierLabelArea(this);
    mainLayout->addWidget(m_tierLabelArea);

    m_chartSplitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(m_chartSplitter, 1);

    m_boundaryOverlay = new BoundaryOverlayWidget(m_viewport, this);
    m_boundaryOverlay->trackWidget(m_chartSplitter);

    connect(m_viewport, &ViewportController::viewportChanged,
            m_timeRuler, &TimeRulerWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged,
            m_boundaryOverlay, &BoundaryOverlayWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged,
            m_miniMap, &MiniMapScrollBar::setViewport);
}

AudioVisualizerContainer::~AudioVisualizerContainer() = default;

ViewportController *AudioVisualizerContainer::viewport() const {
    return m_viewport;
}

IBoundaryModel *AudioVisualizerContainer::boundaryModel() const {
    return m_boundaryModel;
}

TimeRulerWidget *AudioVisualizerContainer::timeRuler() const {
    return m_timeRuler;
}

TierLabelArea *AudioVisualizerContainer::tierLabelArea() const {
    return m_tierLabelArea;
}

QSplitter *AudioVisualizerContainer::chartSplitter() const {
    return m_chartSplitter;
}

MiniMapScrollBar *AudioVisualizerContainer::miniMap() const {
    return m_miniMap;
}

void AudioVisualizerContainer::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
    m_tierLabelArea->setBoundaryModel(model);
}

void AudioVisualizerContainer::setTotalDuration(double seconds) {
    m_viewport->setTotalDuration(seconds);
}

void AudioVisualizerContainer::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_miniMap->setAudioData(samples, sampleRate);
}

void AudioVisualizerContainer::addChart(const QString &id, QWidget *widget,
                                         int defaultOrder, int stretchFactor) {
    ChartEntry entry{id, widget, defaultOrder, stretchFactor};
    m_charts[id] = entry;

    if (!m_chartOrder.contains(id))
        m_chartOrder.append(id);

    connectViewportToWidget(widget);
    rebuildChartLayout();
}

void AudioVisualizerContainer::removeChart(const QString &id) {
    m_charts.remove(id);
    m_chartOrder.removeAll(id);
    rebuildChartLayout();
}

QStringList AudioVisualizerContainer::chartOrder() const {
    return m_chartOrder;
}

void AudioVisualizerContainer::setChartOrder(const QStringList &order) {
    m_chartOrder = order;
    static const SettingsKey<QString> kChartOrder("Layout/chartOrder", "");
    m_settings.set(kChartOrder, order.join(QLatin1Char(',')));
    rebuildChartLayout();
    emit chartOrderChanged(order);
}

QByteArray AudioVisualizerContainer::saveSplitterState() const {
    return m_chartSplitter->saveState();
}

void AudioVisualizerContainer::restoreSplitterState(const QByteArray &state) {
    if (!state.isEmpty())
        m_chartSplitter->restoreState(state);
}

void AudioVisualizerContainer::setPlayWidget(PlayWidget *playWidget) {
    m_playWidget = playWidget;
}

void AudioVisualizerContainer::rebuildChartLayout() {
    while (m_chartSplitter->count() > 0) {
        auto *w = m_chartSplitter->widget(0);
        w->setParent(nullptr);
    }

    for (int i = 0; i < m_chartOrder.size(); ++i) {
        const auto &id = m_chartOrder[i];
        auto it = m_charts.find(id);
        if (it == m_charts.end())
            continue;
        m_chartSplitter->addWidget(it->widget);
        m_chartSplitter->setStretchFactor(i, it->stretchFactor);
        it->widget->show();
    }
}

void AudioVisualizerContainer::connectViewportToWidget(QWidget * /*widget*/) {
}

} // namespace dstools
