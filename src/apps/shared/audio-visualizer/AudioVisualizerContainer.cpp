#include "AudioVisualizerContainer.h"
#include "MiniMapScrollBar.h"
#include "TierLabelArea.h"

#include <ui/BoundaryDragController.h>
#include <ui/BoundaryOverlayWidget.h>
#include <ui/IBoundaryModel.h>

#include <dsfw/widgets/PlayWidget.h>
#include <dstools/TimePos.h>

#include <QLabel>
#include <QMetaMethod>
#include <QMouseEvent>
#include <QPointer>
#include <QResizeEvent>
#include <QTimer>
#include <QWheelEvent>
#include <algorithm>

namespace dstools {

    using dsfw::widgets::PlayWidget;
    using dsfw::widgets::ViewportController;
    using dsfw::widgets::ViewportState;

    AppSettings &AudioVisualizerContainer::chartLayoutSettings() {
        static AppSettings s_settings("AudioVisualizer");
        return s_settings;
    }

    AudioVisualizerContainer::AudioVisualizerContainer(const QString &settingsAppName, QWidget *parent)
        : QWidget(parent), m_settings(settingsAppName) {
        m_viewport = new ViewportController(this);
        m_dragController = new BoundaryDragController(this);

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
        m_scaleLabel->setStyleSheet(QStringLiteral("QLabel {"
                                                   "  color: #aaa;"
                                                   "  background-color: rgba(30, 30, 30, 180);"
                                                   "  border: 1px solid #555;"
                                                   "  border-radius: 3px;"
                                                   "  padding: 2px 6px;"
                                                   "  font-size: 11px;"
                                                   "}"));
        m_scaleLabel->setAlignment(Qt::AlignCenter);
        m_scaleLabel->adjustSize();

        connect(m_viewport, &ViewportController::viewportChanged, m_timeRuler, &TimeRulerWidget::setViewport);
        connect(m_viewport, &ViewportController::viewportChanged, m_boundaryOverlay,
                &BoundaryOverlayWidget::setViewport);
        connect(m_viewport, &ViewportController::viewportChanged, m_miniMap, &MiniMapScrollBar::setViewport);
        connect(m_viewport, &ViewportController::viewportChanged, this,
                &AudioVisualizerContainer::updateScaleIndicator);

        connect(m_dragController, &BoundaryDragController::dragStarted, this, [this]() { installDragEventFilters(); });
        connect(m_dragController, &BoundaryDragController::dragFinished, this, [this]() { removeDragEventFilters(); });
    }

    void AudioVisualizerContainer::updateScaleIndicator() {
        if (!m_scaleLabel || !m_viewport)
            return;

        int resolution = m_viewport->resolution();
        int sampleRate = m_viewport->sampleRate();

        double visibleSec =
            (sampleRate > 0) ? static_cast<double>(m_chartSplitter->width()) * resolution / sampleRate : 0.0;

        QString text;
        if (visibleSec >= 60.0)
            text = QStringLiteral("%1min visible  (%2 spx)").arg(visibleSec / 60.0, 0, 'f', 1).arg(resolution);
        else if (visibleSec >= 1.0)
            text = QStringLiteral("%1s visible  (%2 spx)").arg(visibleSec, 0, 'f', 1).arg(resolution);
        else if (visibleSec > 0.0)
            text = QStringLiteral("%1ms visible  (%2 spx)").arg(visibleSec * 1000.0, 0, 'f', 0).arg(resolution);

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

    BoundaryDragController *AudioVisualizerContainer::dragController() const {
        return m_dragController;
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
        if (m_tierLabelArea)
            m_tierLabelArea->setBoundaryModel(model);
        m_boundaryOverlay->setBoundaryModel(model);

        for (auto &entry : m_charts) {
            const QMetaObject *mo = entry.widget->metaObject();
            for (int i = 0; i < mo->methodCount(); ++i) {
                QMetaMethod method = mo->method(i);
                if (method.name() == "setBoundaryModel" && method.parameterCount() == 1) {
                    method.invoke(entry.widget, Qt::DirectConnection, Q_ARG(IBoundaryModel *, model));
                    break;
                }
            }
        }

        int tierLabelH = m_tierLabelArea ? m_tierLabelArea->height() : 0;
        m_boundaryOverlay->setTierLabelGeometry(tierLabelH, tierLabelH);
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

    void AudioVisualizerContainer::removeTierLabelArea() {
        if (!m_tierLabelArea)
            return;

        auto *layout = qobject_cast<QVBoxLayout *>(this->layout());
        if (layout) {
            int idx = layout->indexOf(m_tierLabelArea);
            if (idx >= 0)
                layout->removeWidget(m_tierLabelArea);
        }

        m_tierLabelArea->removeEventFilter(this);
        m_tierLabelArea->deleteLater();
        m_tierLabelArea = nullptr;

        if (m_boundaryOverlay) {
            int editH = m_editorWidget ? m_editorWidget->height() : 0;
            m_boundaryOverlay->setTierLabelGeometry(editH, editH);
        }
        updateOverlayTopOffset();

        QTimer::singleShot(0, this, [this]() {
            updateOverlayTopOffset();
            if (m_boundaryOverlay)
                m_boundaryOverlay->forceReposition();
        });
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
        if (m_editorWidget && m_tierLabelArea)
            extraHeight = m_editorWidget->height();
        if (m_boundaryOverlay)
            m_boundaryOverlay->setExtraTopOffset(extraHeight);
    }

    void AudioVisualizerContainer::setTotalDuration(double seconds) {
        m_viewport->setTotalDuration(seconds);
        updateViewRangeFromResolution();
    }

    void AudioVisualizerContainer::setAudioData(const std::vector<float> &samples, int sampleRate) {
        m_miniMap->setAudioData(samples, sampleRate);
    }

    void AudioVisualizerContainer::wheelEvent(QWheelEvent *event) {
        if (event->modifiers() & Qt::ControlModifier) {
            const int delta = event->angleDelta().y();
            if (delta > 0)
                m_viewport->zoomIn(m_viewport->viewCenter());
            else if (delta < 0)
                m_viewport->zoomOut(m_viewport->viewCenter());
            event->accept();
            updateViewRangeFromResolution();
            return;
        }
        QWidget::wheelEvent(event);
    }

    bool AudioVisualizerContainer::eventFilter(QObject *watched, QEvent *event) {
        if (event->type() == QEvent::MouseButtonPress && m_dragController && !m_dragController->isDragging()) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                int tier = m_boundaryModel ? m_boundaryModel->activeTierIndex() : -1;
                if (tier >= 0) {
                    double time = xToTimeGlobal(me->globalPosition().x());
                    TimePos targetTime = secToUs(time);
                    TimePos kSnapThreshold = secToUs(0.03);
                    int count = m_boundaryModel->boundaryCount(tier);
                    int bestIdx = -1;
                    TimePos bestDist = kSnapThreshold;
                    for (int b = 0; b < count; ++b) {
                        TimePos dist = std::abs(m_boundaryModel->boundaryTime(tier, b) - targetTime);
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestIdx = b;
                        }
                    }
                    if (bestIdx >= 0) {
                        m_dragController->startDrag(tier, bestIdx, nullptr);
                        return true;
                    }
                }
            }
        }

        if (m_dragController && m_dragController->isDragging()) {
            if (event->type() == QEvent::MouseMove) {
                auto *me = static_cast<QMouseEvent *>(event);
                double time = xToTimeGlobal(me->globalPosition().x());
                m_dragController->updateDrag(secToUs(time));
                invalidateBoundaryModel();
                return true;
            }
            if (event->type() == QEvent::MouseButtonRelease) {
                auto *me = static_cast<QMouseEvent *>(event);
                if (me->button() == Qt::LeftButton) {
                    double time = xToTimeGlobal(me->globalPosition().x());
                    m_dragController->endDrag(secToUs(time), nullptr);
                    invalidateBoundaryModel();
                    return true;
                }
            }
        }

        if (watched == m_editorWidget && event->type() == QEvent::Resize) {
            updateOverlayTopOffset();
        }
        if (event->type() == QEvent::Resize) {
            if (watched == m_chartSplitter)
                updateScaleIndicator();
        }
        if (watched == m_tierLabelArea && event->type() == QEvent::Resize) {
            m_boundaryOverlay->setTierLabelGeometry(m_tierLabelArea->height(), m_tierLabelArea->height());
            updateOverlayTopOffset();
        }
        return QWidget::eventFilter(watched, event);
    }

    void AudioVisualizerContainer::setDefaultResolution(int resolution) {
        m_defaultResolution = std::clamp(resolution, 10, 44100);
        // Apply immediately so TimeRuler/widgets show correct initial scale
        m_viewport->setResolution(m_defaultResolution);
    }

    void AudioVisualizerContainer::saveResolution() {
        static const dstools::SettingsKey<int> kResolution("Viewport/resolution", 0);
        m_settings.set(kResolution, m_viewport->resolution());
    }

    bool AudioVisualizerContainer::restoreResolution() {
        static const dstools::SettingsKey<int> kResolution("Viewport/resolution", 0);
        int saved = m_settings.get(kResolution);
        if (saved > 0) {
            m_viewport->setResolution(saved);
            return true;
        }
        return false;
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

        // fitToWindow is called when new audio is loaded — always use the
        // page's default resolution so the initial view matches design intent.
        // Saved per-page resolution is restored separately in onActivated()
        // (page re-entry), not here.
        m_viewport->setResolution(m_defaultResolution);
        updateViewRangeFromResolution();
    }

    void AudioVisualizerContainer::updateViewRangeFromResolution() {
        int w = m_chartSplitter ? m_chartSplitter->width() : width();
        if (w <= 0 || m_viewport->sampleRate() <= 0)
            return;

        // visibleDuration = widgetWidth * resolution / sampleRate
        double visibleDuration = static_cast<double>(w) * m_viewport->resolution() / m_viewport->sampleRate();
        double totalDur = m_viewport->totalDuration();
        double startSec = m_viewport->startSec();

        // Clamp: don't exceed audio end
        double endSec = startSec + visibleDuration;
        if (totalDur > 0.0 && endSec > totalDur) {
            endSec = totalDur;
            startSec = endSec - visibleDuration;
            if (startSec < 0.0)
                startSec = 0.0;
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
        if (!m_pendingSplitterState.isEmpty() && m_chartSplitter && m_chartSplitter->height() > 0) {
            m_chartSplitter->restoreState(m_pendingSplitterState);
            m_pendingSplitterState.clear();
            bool hasZeroHeight = false;
            for (int i = 0; i < m_chartSplitter->count(); ++i) {
                if (m_chartSplitter->widget(i)->isVisible() && m_chartSplitter->sizes().at(i) == 0) {
                    hasZeroHeight = true;
                    break;
                }
            }
            if (hasZeroHeight)
                applyDefaultHeightRatios();
        }
        updateScaleIndicator();
    }

    void AudioVisualizerContainer::addChart(const QString &id, QWidget *widget, int defaultOrder, int stretchFactor,
                                            double heightWeight) {
        ChartEntry entry{id, widget, defaultOrder, stretchFactor, heightWeight};
        m_charts[id] = entry;

        if (!m_chartOrder.contains(id))
            m_chartOrder.append(id);

        connectViewportToWidget(widget);

        // Auto-set drag controller on chart widgets that support it (reduces boilerplate
        // in page assembly code; see refactoring-roadmap-v2.md §7.7).
        if (m_dragController) {
            const QMetaObject *mo = widget->metaObject();
            for (int i = 0; i < mo->methodCount(); ++i) {
                QMetaMethod method = mo->method(i);
                if (method.name() == "setDragController" && method.parameterCount() == 1) {
                    method.invoke(widget, Qt::DirectConnection, Q_ARG(BoundaryDragController *, m_dragController));
                    break;
                }
            }
        }

        if (m_boundaryModel) {
            const QMetaObject *mo = widget->metaObject();
            for (int i = 0; i < mo->methodCount(); ++i) {
                QMetaMethod method = mo->method(i);
                if (method.name() == "setBoundaryModel" && method.parameterCount() == 1) {
                    method.invoke(widget, Qt::DirectConnection, Q_ARG(IBoundaryModel *, m_boundaryModel));
                    break;
                }
            }
        }

        widget->installEventFilter(this);

        restoreChartOrder();

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
        saveChartOrder();
        rebuildChartLayout();
        emit chartOrderChanged(order);
    }

    void AudioVisualizerContainer::setChartVisible(const QString &id, bool visible) {
        auto it = m_charts.find(id);
        if (it == m_charts.end())
            return;

        if (visible)
            m_hiddenCharts.remove(id);
        else
            m_hiddenCharts.insert(id);

        it->widget->setVisible(visible);
        rebuildChartLayout();
        emit chartVisibilityChanged(id, visible);
    }

    bool AudioVisualizerContainer::chartVisible(const QString &id) const {
        return !m_hiddenCharts.contains(id);
    }

    void AudioVisualizerContainer::saveChartVisibility() {
        static const dstools::SettingsKey<QString> kChartVisible("ViewLayout/chartVisible", "");
        QStringList visible;
        for (const auto &id : m_chartOrder) {
            if (!m_hiddenCharts.contains(id))
                visible.append(id);
        }
        chartLayoutSettings().set(kChartVisible, visible.join(QLatin1Char(',')));
    }

    void AudioVisualizerContainer::restoreChartVisibility() {
        static const dstools::SettingsKey<QString> kChartVisible("ViewLayout/chartVisible", "");
        chartLayoutSettings().reload();
        QString saved = chartLayoutSettings().get(kChartVisible);
        if (saved.isEmpty())
            return;

        QStringList visibleIds = saved.split(QLatin1Char(','), Qt::SkipEmptyParts);
        QSet<QString> visibleSet(visibleIds.begin(), visibleIds.end());

        for (const auto &id : m_chartOrder) {
            bool shouldBeVisible = visibleSet.contains(id);
            if (shouldBeVisible)
                m_hiddenCharts.remove(id);
            else
                m_hiddenCharts.insert(id);

            auto it = m_charts.find(id);
            if (it != m_charts.end())
                it->widget->setVisible(shouldBeVisible);
        }
        rebuildChartLayout();
    }

    void AudioVisualizerContainer::saveChartOrder() {
        static const dstools::SettingsKey<QString> kChartOrder("ViewLayout/chartOrder", "");
        chartLayoutSettings().set(kChartOrder, m_chartOrder.join(QLatin1Char(',')));
    }

    bool AudioVisualizerContainer::restoreChartOrder() {
        static const dstools::SettingsKey<QString> kChartOrder("ViewLayout/chartOrder", "");
        chartLayoutSettings().reload();
        QString saved = chartLayoutSettings().get(kChartOrder);
        if (saved.isEmpty())
            return false;

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
        return true;
    }

    QByteArray AudioVisualizerContainer::saveSplitterState() const {
        return m_chartSplitter->saveState();
    }

    void AudioVisualizerContainer::restoreSplitterState(const QByteArray &state) {
        if (state.isEmpty())
            return;
        if (m_chartSplitter && m_chartSplitter->height() > 0) {
            m_chartSplitter->restoreState(state);
            bool hasZeroHeight = false;
            for (int i = 0; i < m_chartSplitter->count(); ++i) {
                if (m_chartSplitter->widget(i)->isVisible() && m_chartSplitter->sizes().at(i) == 0) {
                    hasZeroHeight = true;
                    break;
                }
            }
            if (hasZeroHeight)
                applyDefaultHeightRatios();
        } else {
            m_pendingSplitterState = state;
        }
    }

    void AudioVisualizerContainer::setPlayWidget(PlayWidget *playWidget) {
        if (m_playWidget == playWidget)
            return;

        // Disconnect previous
        if (m_playWidget) {
            disconnect(m_playWidget, nullptr, this, nullptr);
        }
        m_playWidget = playWidget;

        if (!m_playWidget)
            return;

        // Create playhead timer (auto-clear after 200ms of no updates)
        if (!m_playheadTimer) {
            m_playheadTimer = new QTimer(this);
            m_playheadTimer->setSingleShot(true);
            m_playheadTimer->setInterval(200);
        }

        // Forward playhead to all registered chart widgets via QMetaMethod invocation
        connect(m_playWidget, &PlayWidget::playheadChanged, this, [this](double sec) {
            // Iterate all chart widgets and call setPlayhead if they have the method
            for (const auto &id : m_chartOrder) {
                auto it = m_charts.find(id);
                if (it == m_charts.end() || !it->widget)
                    continue;
                const QMetaObject *mo = it->widget->metaObject();
                for (int i = 0; i < mo->methodCount(); ++i) {
                    QMetaMethod method = mo->method(i);
                    if (method.name() == "setPlayhead" && method.parameterCount() == 1) {
                        double secCopy = sec;
                        method.invoke(it->widget, Qt::DirectConnection, QGenericArgument("double", &secCopy));
                        break;
                    }
                }
            }
            // Also forward to boundary overlay
            if (m_boundaryOverlay)
                m_boundaryOverlay->setPlayhead(sec);
            // Reset auto-clear timer
            if (m_playheadTimer)
                m_playheadTimer->start();
        });

        // Auto-clear playhead when playback stops
        connect(m_playheadTimer, &QTimer::timeout, this, [this]() {
            for (const auto &id : m_chartOrder) {
                auto it = m_charts.find(id);
                if (it == m_charts.end() || !it->widget)
                    continue;
                const QMetaObject *mo = it->widget->metaObject();
                for (int i = 0; i < mo->methodCount(); ++i) {
                    QMetaMethod method = mo->method(i);
                    if (method.name() == "setPlayhead" && method.parameterCount() == 1) {
                        double negOne = -1.0;
                        method.invoke(it->widget, Qt::DirectConnection, QGenericArgument("double", &negOne));
                        break;
                    }
                }
            }
            if (m_boundaryOverlay)
                m_boundaryOverlay->setPlayhead(-1.0);
        });
    }

    void AudioVisualizerContainer::connectPlayheadToWidget(QWidget *chart) {
        // Explicit connection for charts added after setPlayWidget.
        // The setPlayWidget already handles all registered charts via iteration,
        // so this is a no-op if setPlayWidget was called first.
        Q_UNUSED(chart)
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

        int splitterIndex = 0;
        for (int i = 0; i < m_chartOrder.size(); ++i) {
            const auto &id = m_chartOrder[i];
            auto it = m_charts.find(id);
            if (it == m_charts.end())
                continue;
            if (m_hiddenCharts.contains(id)) {
                it->widget->hide();
                continue;
            }
            m_chartSplitter->addWidget(it->widget);
            m_chartSplitter->setStretchFactor(splitterIndex, it->stretchFactor);
            it->widget->show();
            ++splitterIndex;
        }

        // Apply default height ratios — per-page state is restored externally
        applyDefaultHeightRatios();
    }

    void AudioVisualizerContainer::applyDefaultHeightRatios() {
        int totalHeight = m_chartSplitter->height();
        if (totalHeight <= 0)
            totalHeight = 600; // fallback before first show

        double totalWeight = 0.0;
        for (const auto &id : m_chartOrder) {
            if (m_hiddenCharts.contains(id))
                continue;
            auto it = m_charts.find(id);
            if (it != m_charts.end())
                totalWeight += it->heightWeight;
        }
        if (totalWeight <= 0.0)
            return;

        QList<int> sizes;
        for (const auto &id : m_chartOrder) {
            if (m_hiddenCharts.contains(id))
                continue;
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
        connect(m_viewport, &ViewportController::viewportChanged, this, [safeWidget](const ViewportState &state) {
            if (!safeWidget)
                return;
            const QMetaObject *mo = safeWidget->metaObject();
            for (int i = 0; i < mo->methodCount(); ++i) {
                QMetaMethod method = mo->method(i);
                if (method.name() == "setViewport" && method.parameterCount() == 1) {
                    method.invoke(safeWidget.data(), Qt::DirectConnection,
                                  QGenericArgument(method.parameterTypes().at(0).constData(), &state));
                    break;
                }
            }
        });
    }

    double AudioVisualizerContainer::xToTimeGlobal(qreal globalX) const {
        if (!m_chartSplitter)
            return 0.0;
        qreal localX = globalX - m_chartSplitter->mapToGlobal(QPoint(0, 0)).x();
        double viewDuration = m_viewport->endSec() - m_viewport->startSec();
        if (viewDuration <= 0.0 || m_chartSplitter->width() <= 0)
            return m_viewport->startSec();
        return m_viewport->startSec() + viewDuration * localX / m_chartSplitter->width();
    }

    void AudioVisualizerContainer::installDragEventFilters() {
        if (m_editorWidget)
            m_editorWidget->installEventFilter(this);
    }

    void AudioVisualizerContainer::removeDragEventFilters() {
        if (m_editorWidget)
            m_editorWidget->removeEventFilter(this);
    }

} // namespace dstools
