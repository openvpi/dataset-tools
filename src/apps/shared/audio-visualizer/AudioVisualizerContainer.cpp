#include "AudioVisualizerContainer.h"
#include "MiniMapScrollBar.h"
#include "TierLabelArea.h"

#include <ui/BoundaryOverlayWidget.h>
#include <ui/IBoundaryModel.h>

#include <dsfw/widgets/PlayWidget.h>

#include <QMetaMethod>
#include <QPointer>
#include <QSettings>
#include <algorithm>

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
    m_tierLabelArea->setViewportController(m_viewport);
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

BoundaryOverlayWidget *AudioVisualizerContainer::boundaryOverlay() const {
    return m_boundaryOverlay;
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
    m_boundaryOverlay->setBoundaryModel(model);
    m_boundaryOverlay->setTierLabelGeometry(m_tierLabelArea->height(), m_tierLabelArea->height());
    updateOverlayTopOffset();
}

void AudioVisualizerContainer::setTierLabelArea(TierLabelArea *area) {
    if (!area || area == m_tierLabelArea)
        return;

    auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
    if (!layout)
        return;

    int idx = layout->indexOf(m_tierLabelArea);
    if (idx < 0)
        return;

    layout->removeWidget(m_tierLabelArea);
    m_tierLabelArea->deleteLater();

    m_tierLabelArea = area;
    m_tierLabelArea->setViewportController(m_viewport);
    if (m_boundaryModel)
        m_tierLabelArea->setBoundaryModel(m_boundaryModel);
    layout->insertWidget(idx, m_tierLabelArea);
    m_boundaryOverlay->setTierLabelGeometry(m_tierLabelArea->height(), m_tierLabelArea->height());
    updateOverlayTopOffset();
}

void AudioVisualizerContainer::setEditorWidget(QWidget *widget) {
    if (!widget || widget == m_editorWidget)
        return;

    auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
    if (!layout)
        return;

    // If there's an existing editor widget, replace it
    if (m_editorWidget) {
        int oldIdx = layout->indexOf(m_editorWidget);
        if (oldIdx >= 0) {
            layout->removeWidget(m_editorWidget);
            m_editorWidget->deleteLater();
        }
    }

    m_editorWidget = widget;

    // Insert editor widget before the chart splitter
    int splitterIdx = layout->indexOf(m_chartSplitter);
    if (splitterIdx >= 0) {
        layout->insertWidget(splitterIdx, m_editorWidget);
    } else {
        layout->addWidget(m_editorWidget);
    }

    // Connect viewport to editor widget
    connectViewportToWidget(m_editorWidget);

    // Update the boundary overlay so it covers the tier label + editor area
    // (the overlay tracks chartSplitter and extends upward by this offset)
    updateOverlayTopOffset();

    // Track editor widget height changes to keep overlay covering it
    m_editorWidget->installEventFilter(this);
}

void AudioVisualizerContainer::updateOverlayTopOffset() {
    int extraHeight = 0;
    if (m_editorWidget)
        extraHeight = m_editorWidget->height();
    if (m_boundaryOverlay)
        m_boundaryOverlay->setExtraTopOffset(extraHeight);
}

void AudioVisualizerContainer::setTotalDuration(double seconds) {
    m_viewport->setTotalDuration(seconds);
}

void AudioVisualizerContainer::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_miniMap->setAudioData(samples, sampleRate);
}

bool AudioVisualizerContainer::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_editorWidget && event->type() == QEvent::Resize) {
        updateOverlayTopOffset();
    }
    return QWidget::eventFilter(watched, event);
}

void AudioVisualizerContainer::fitToWindow() {
    int64_t totalSamples = m_viewport->totalSamples();
    if (totalSamples <= 0 || width() <= 0)
        return;
    int fitResolution = static_cast<int>(totalSamples / width());
    fitResolution = std::clamp(fitResolution, 10, 400);
    m_viewport->setResolution(fitResolution);
    m_viewport->setViewRange(0, m_viewport->totalDuration());
}

void AudioVisualizerContainer::addChart(const QString &id, QWidget *widget,
                                         int defaultOrder, int stretchFactor,
                                         double heightWeight) {
    ChartEntry entry{id, widget, defaultOrder, stretchFactor, heightWeight};
    m_charts[id] = entry;

    if (!m_chartOrder.contains(id))
        m_chartOrder.append(id);

    connectViewportToWidget(widget);

    QSettings settings;
    QString saved = settings.value(QStringLiteral("AudioVisualizer/chartOrder")).toString();
    if (!saved.isEmpty()) {
        QStringList savedOrder = saved.split(QLatin1Char(','), Qt::SkipEmptyParts);
        QStringList reordered;
        for (const auto &chartId : savedOrder) {
            if (m_chartOrder.contains(chartId))
                reordered.append(chartId);
        }
        for (const auto &chartId : m_chartOrder) {
            if (!reordered.contains(chartId))
                reordered.append(chartId);
        }
        m_chartOrder = reordered;
    }

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
    QSettings settings;
    settings.setValue(QStringLiteral("AudioVisualizer/chartOrder"), order.join(QLatin1Char(',')));
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

void AudioVisualizerContainer::invalidateBoundaryModel() {
    if (m_boundaryOverlay)
        m_boundaryOverlay->update();

    if (m_tierLabelArea)
        m_tierLabelArea->onModelDataChanged();

    for (const auto &id : m_chartOrder) {
        auto it = m_charts.find(id);
        if (it != m_charts.end() && it->widget)
            it->widget->update();
    }

    emit boundaryModelInvalidated();
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

    // Apply default height ratios if no saved splitter state exists
    QSettings settings;
    QByteArray savedState = settings.value(
        QStringLiteral("AudioVisualizer/splitterState")).toByteArray();
    if (savedState.isEmpty()) {
        applyDefaultHeightRatios();
    }
}

void AudioVisualizerContainer::applyDefaultHeightRatios() {
    int totalHeight = m_chartSplitter->height();
    if (totalHeight <= 0)
        totalHeight = 600; // fallback before first show

    double totalWeight = 0.0;
    for (const auto &id : m_chartOrder) {
        auto it = m_charts.find(id);
        if (it != m_charts.end())
            totalWeight += it->heightWeight;
    }
    if (totalWeight <= 0.0)
        return;

    QList<int> sizes;
    for (const auto &id : m_chartOrder) {
        auto it = m_charts.find(id);
        if (it != m_charts.end())
            sizes.append(static_cast<int>(totalHeight * it->heightWeight / totalWeight));
    }
    m_chartSplitter->setSizes(sizes);
}

void AudioVisualizerContainer::connectViewportToWidget(QWidget *widget) {
    if (!widget || !m_viewport)
        return;

    QPointer<QWidget> safeWidget(widget);
    connect(m_viewport, &ViewportController::viewportChanged, this,
            [safeWidget](const ViewportState &state) {
                if (!safeWidget)
                    return;
                const QMetaObject *mo = safeWidget->metaObject();
                for (int i = 0; i < mo->methodCount(); ++i) {
                    QMetaMethod method = mo->method(i);
                    if (method.name() == "setViewport" && method.parameterCount() == 1) {
                        method.invoke(safeWidget.data(), Qt::DirectConnection,
                                      QGenericArgument(method.parameterTypes().at(0).constData(),
                                                      &state));
                        break;
                    }
                }
            });
}

} // namespace dstools
