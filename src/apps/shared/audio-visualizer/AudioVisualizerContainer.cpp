#include "AudioVisualizerContainer.h"
#include "MiniMapScrollBar.h"
#include "TierLabelArea.h"

#include <ui/BoundaryOverlayWidget.h>
#include <ui/IBoundaryModel.h>

#include <dsfw/widgets/PlayWidget.h>

#include <QLabel>
#include <QMetaMethod>
#include <QPointer>
#include <QResizeEvent>
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
    m_tierLabelArea->installEventFilter(this);
    mainLayout->addWidget(m_tierLabelArea);

    m_chartSplitter = new QSplitter(Qt::Vertical, this);
    m_chartSplitter->installEventFilter(this);
    mainLayout->addWidget(m_chartSplitter, 1);

    m_boundaryOverlay = new BoundaryOverlayWidget(m_viewport, this);
    m_boundaryOverlay->trackWidget(m_chartSplitter);

    // Scale indicator at the bottom-right of the chart area
    m_scaleLabel = new QLabel(this);
    m_scaleLabel->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  color: #aaa;"
        "  background-color: rgba(30, 30, 30, 180);"
        "  border: 1px solid #555;"
        "  border-radius: 3px;"
        "  padding: 2px 6px;"
        "  font-size: 11px;"
        "}"));
    m_scaleLabel->setAlignment(Qt::AlignCenter);
    m_scaleLabel->adjustSize();

    connect(m_viewport, &ViewportController::viewportChanged,
            m_timeRuler, &TimeRulerWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged,
            m_boundaryOverlay, &BoundaryOverlayWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged,
            m_miniMap, &MiniMapScrollBar::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged,
            this, &AudioVisualizerContainer::updateScaleIndicator);
}

void AudioVisualizerContainer::updateScaleIndicator() {
    if (!m_scaleLabel || !m_viewport)
        return;

    int resolution = m_viewport->resolution();
    int sampleRate = m_viewport->sampleRate();

    // msPerDiv: time represented by ~80 pixels at current resolution
    double msPerDiv = (sampleRate > 0)
        ? static_cast<double>(resolution) / sampleRate * 80.0 * 1000.0
        : 0.0;

    // Format: show time-per-division and resolution
    QString text;
    if (msPerDiv >= 1000.0)
        text = QStringLiteral("%1s/div  (%2 spx)")
            .arg(msPerDiv / 1000.0, 0, 'f', 1)
            .arg(resolution);
    else if (msPerDiv >= 1.0)
        text = QStringLiteral("%1ms/div  (%2 spx)")
            .arg(msPerDiv, 0, 'f', 0)
            .arg(resolution);
    else
        text = QStringLiteral("%1μs/div  (%2 spx)")
            .arg(msPerDiv * 1000.0, 0, 'f', 0)
            .arg(resolution);

    m_scaleLabel->setText(text);
    m_scaleLabel->adjustSize();

    // Position at bottom-right of the chart splitter area
    int x = m_chartSplitter->width() - m_scaleLabel->width() - 8;
    int y = m_chartSplitter->y() + m_chartSplitter->height() - m_scaleLabel->height() - 8;
    m_scaleLabel->move(x, y);
    m_scaleLabel->raise();
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
    m_tierLabelArea->installEventFilter(this);
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
    if (event->type() == QEvent::Resize) {
        // Update scale label and overlay position on any resize
        if (watched == m_chartSplitter)
            updateScaleIndicator();
    }
    // When the tier label area resizes (e.g. after rebuildRadioButtons changes height),
    // update the overlay geometry so boundary lines align correctly
    if (watched == m_tierLabelArea && event->type() == QEvent::Resize) {
        m_boundaryOverlay->setTierLabelGeometry(m_tierLabelArea->height(),
                                                 m_tierLabelArea->height());
        updateOverlayTopOffset();
    }
    return QWidget::eventFilter(watched, event);
}

void AudioVisualizerContainer::setDefaultResolution(int resolution) {
    m_defaultResolution = std::clamp(resolution, 10, 400);
}

void AudioVisualizerContainer::fitToWindow() {
    int64_t totalSamples = m_viewport->totalSamples();
    if (totalSamples <= 0)
        return;

    int w = m_chartSplitter ? m_chartSplitter->width() : width();
    if (w <= 0) {
        m_needsFitOnResize = true;
        return;
    }
    m_needsFitOnResize = false;

    // Apply the configured default resolution
    m_viewport->setResolution(m_defaultResolution);
    // Compute and set the correct view range for this resolution + width
    updateViewRangeFromResolution();
}

void AudioVisualizerContainer::updateViewRangeFromResolution() {
    int w = m_chartSplitter ? m_chartSplitter->width() : width();
    if (w <= 0 || m_viewport->sampleRate() <= 0)
        return;

    // visibleDuration = widgetWidth * resolution / sampleRate
    double visibleDuration = static_cast<double>(w) * m_viewport->resolution()
                             / m_viewport->sampleRate();
    double totalDur = m_viewport->totalDuration();
    double startSec = m_viewport->startSec();

    // Clamp: don't exceed audio end
    double endSec = startSec + visibleDuration;
    if (totalDur > 0.0 && endSec > totalDur) {
        endSec = totalDur;
        startSec = endSec - visibleDuration;
        if (startSec < 0.0) startSec = 0.0;
    }

    m_viewport->setViewRange(startSec, endSec);
}

void AudioVisualizerContainer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (m_needsFitOnResize && width() > 0) {
        fitToWindow();
    } else if (m_viewport->totalSamples() > 0) {
        // On resize, recalculate view range to match new width at current resolution
        updateViewRangeFromResolution();
    }
    updateScaleIndicator();
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
