#include "WaveformPanel.h"
#include "PlaybackController.h"

#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

#include <algorithm>
#include <cmath>

namespace dstools {
namespace waveform {

// ─── TimeRuler (internal) ─────────────────────────────────────────────────────

class WaveformPanel::TimeRuler : public QWidget {
public:
    explicit TimeRuler(ViewportController * /*viewport*/, QWidget *parent = nullptr)
        : QWidget(parent) {
        setFixedHeight(24);
    }

    void setViewport(const ViewportState &state) {
        m_viewStart = state.startSec;
        m_viewEnd = state.endSec;
        m_pixelsPerSecond = state.pixelsPerSecond;
        update();
    }

protected:
    void paintEvent(QPaintEvent * /*event*/) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(35, 35, 40));

        double viewDuration = m_viewEnd - m_viewStart;
        if (viewDuration <= 0.0)
            return;

        // Choose tick interval (~150px apart)
        double secPerTick = 150.0 / m_pixelsPerSecond;
        static const double niceSteps[] = {0.001, 0.002, 0.005, 0.01,  0.02,  0.05,
                                           0.1,   0.2,   0.5,   1.0,   2.0,   5.0,
                                           10.0,  20.0,  30.0,  60.0,  120.0, 300.0,
                                           600.0, 1800.0, 3600.0};
        double majorInterval = niceSteps[std::size(niceSteps) - 1]; // fallback to largest
        for (double step : niceSteps) {
            if (step >= secPerTick) {
                majorInterval = step;
                break;
            }
        }

        double minorInterval = majorInterval / 5.0;

        // Minor ticks
        painter.setPen(QPen(QColor(70, 70, 80), 1));
        double firstMinor = std::floor(m_viewStart / minorInterval) * minorInterval;
        for (double t = firstMinor; t <= m_viewEnd; t += minorInterval) {
            if (t < m_viewStart)
                continue;
            int x = timeToX(t);
            painter.drawLine(x, height() - 4, x, height());
        }

        // Major ticks + labels
        QFont font = painter.font();
        font.setPixelSize(10);
        painter.setFont(font);

        double firstMajor = std::floor(m_viewStart / majorInterval) * majorInterval;
        for (double t = firstMajor; t <= m_viewEnd; t += majorInterval) {
            if (t < m_viewStart)
                continue;
            int x = timeToX(t);

            painter.setPen(QPen(QColor(120, 120, 140), 1));
            painter.drawLine(x, height() - 10, x, height());

            painter.setPen(QColor(180, 180, 200));
            QString label;
            if (majorInterval >= 1.0) {
                int totalSec = static_cast<int>(t);
                int min = totalSec / 60;
                int sec = totalSec % 60;
                if (min > 0)
                    label = QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
                else
                    label = QString::number(totalSec) + "s";
            } else if (majorInterval >= 0.01) {
                label = QString::number(t, 'f', 2) + "s";
            } else {
                label = QString::number(t, 'f', 3) + "s";
            }

            QRect textRect(x - 40, 0, 80, height() - 10);
            painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, label);
        }

        // Bottom line
        painter.setPen(QPen(QColor(80, 80, 100), 1));
        painter.drawLine(0, height() - 1, width(), height() - 1);
    }

private:
    int timeToX(double time) const {
        double viewDuration = m_viewEnd - m_viewStart;
        if (viewDuration <= 0.0 || width() <= 0)
            return 0;
        return static_cast<int>((time - m_viewStart) / viewDuration * width());
    }

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;
};

// ─── WaveformDisplay (internal) ───────────────────────────────────────────────

class WaveformPanel::WaveformDisplay : public QWidget {
    Q_OBJECT

public:
    explicit WaveformDisplay(ViewportController *viewport, QWidget *parent = nullptr)
        : QWidget(parent), m_viewport(viewport) {
        setMouseTracking(true);
        setMinimumHeight(150);
    }

    void setSamples(const std::vector<float> *samples, int sampleRate) {
        m_samples = samples;
        m_sampleRate = sampleRate;
        rebuildCache();
        update();
    }

    void setBoundaries(const std::vector<BoundaryInfo> *boundaries) {
        m_boundaries = boundaries;
        update();
    }

    void setViewport(const ViewportState &state) {
        m_viewStart = state.startSec;
        m_viewEnd = state.endSec;
        m_pixelsPerSecond = state.pixelsPerSecond;
        rebuildCache();
        update();
    }

    void setPlayhead(double sec) {
        m_playhead = sec;
        update();
    }

    void setToolMode(ToolMode mode) {
        m_toolMode = mode;
        if (mode == ToolMode::Knife)
            setCursor(Qt::CrossCursor);
        else
            setCursor(Qt::ArrowCursor);
    }

    void setSelectedBoundary(int index) {
        m_selectedBoundary = index;
        update();
    }

signals:
    void positionClicked(double sec);
    void rightClickPlay(double sec);
    void zoomRequested(double atTime, double factor);
    void boundaryClicked(int index);
    void boundaryDragged(int index, double oldTime, double newTime);
    void boundaryRightClicked(int index);

protected:
    void paintEvent(QPaintEvent * /*event*/) override {
        QPainter painter(this);
        painter.fillRect(rect(), QColor(30, 30, 30));

        if (!m_samples || m_samples->empty()) {
            painter.setPen(Qt::gray);
            painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
            return;
        }

        drawWaveform(painter);
        drawBoundaries(painter);
        drawPlayCursor(painter);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            int idx = hitTestBoundary(event->pos().x());
            if (m_toolMode == ToolMode::Pointer) {
                if (idx >= 0) {
                    // Start dragging this boundary
                    m_dragging = true;
                    m_dragIndex = idx;
                    m_dragOriginalTime = (*m_boundaries)[idx].timeSec;
                    m_selectedBoundary = idx;
                    emit boundaryClicked(idx);
                    setCursor(Qt::SizeHorCursor);
                } else {
                    // Deselect
                    m_selectedBoundary = -1;
                    emit boundaryClicked(-1);
                }
                update();
            } else if (m_toolMode == ToolMode::Knife) {
                // Knife mode: add a slice point at click position
                emit positionClicked(xToTime(event->pos().x()));
            }
        }
        QWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (m_dragging) {
            // Update cursor position during drag — visual feedback only
            m_dragCurrentTime = xToTime(event->pos().x());
            update();
        } else if (m_toolMode == ToolMode::Pointer) {
            // Hover: change cursor when over a boundary
            int idx = hitTestBoundary(event->pos().x());
            setCursor(idx >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton && m_dragging) {
            m_dragging = false;
            double newTime = xToTime(event->pos().x());
            if (newTime < 0.0)
                newTime = 0.0;
            if (std::abs(newTime - m_dragOriginalTime) > 0.001) {
                emit boundaryDragged(m_dragIndex, m_dragOriginalTime, newTime);
            }
            setCursor(Qt::ArrowCursor);
            update();
        }
        QWidget::mouseReleaseEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent *event) override {
        // Check if right-clicking on a boundary (in Pointer mode → delete)
        if (m_toolMode == ToolMode::Pointer) {
            int idx = hitTestBoundary(event->pos().x());
            if (idx >= 0) {
                emit boundaryRightClicked(idx);
                event->accept();
                return;
            }
        }
        // Otherwise: right-click = direct play (find surrounding boundaries)
        double clickTime = xToTime(event->pos().x());
        emit rightClickPlay(clickTime);
        event->accept();
    }

    void wheelEvent(QWheelEvent *event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            double factor = (event->angleDelta().y() > 0) ? 1.25 : 0.8;
            double atTime = xToTime(static_cast<int>(event->position().x()));
            emit zoomRequested(atTime, factor);
            event->accept();
        } else {
            QWidget::wheelEvent(event);
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        rebuildCache();
        QWidget::resizeEvent(event);
    }

private:
    void drawWaveform(QPainter &painter) {
        int w = width();
        int h = height();
        int centerY = h / 2;

        painter.setPen(QPen(QColor(60, 60, 60), 1));
        painter.drawLine(0, centerY, w, centerY);

        painter.setPen(QPen(QColor(100, 180, 255), 1));
        int numPixels = static_cast<int>(m_cache.size());
        for (int x = 0; x < w && x < numPixels; ++x) {
            int yMin = centerY - static_cast<int>(m_cache[x].max * centerY);
            int yMax = centerY - static_cast<int>(m_cache[x].min * centerY);
            if (yMin > yMax)
                std::swap(yMin, yMax);
            painter.drawLine(x, yMin, x, yMax);
        }
    }

    void drawBoundaries(QPainter &painter) {
        if (!m_boundaries)
            return;
        for (size_t i = 0; i < m_boundaries->size(); ++i) {
            double t = (*m_boundaries)[i].timeSec;
            // If dragging this boundary, draw at drag position instead
            if (m_dragging && static_cast<int>(i) == m_dragIndex)
                t = m_dragCurrentTime;

            int x = timeToX(t);
            if (x < 0 || x > width())
                continue;

            bool isSelected = (static_cast<int>(i) == m_selectedBoundary);
            if (isSelected) {
                painter.setPen(QPen(QColor(255, 120, 50, 240), 2));
            } else {
                painter.setPen(QPen(QColor(255, 200, 100, 200), 1));
            }
            painter.drawLine(x, 0, x, height());
        }
    }

    void drawPlayCursor(QPainter &painter) {
        if (m_playhead < 0.0)
            return;
        int x = timeToX(m_playhead);
        if (x >= 0 && x <= width()) {
            painter.setPen(QPen(QColor(255, 100, 100), 2));
            painter.drawLine(x, 0, x, height());
        }
    }

    void rebuildCache() {
        m_cache.clear();
        if (!m_samples || m_samples->empty() || m_pixelsPerSecond <= 0.0)
            return;

        int numPixels = qMax(1, width());
        m_cache.resize(numPixels);

        for (int x = 0; x < numPixels; ++x) {
            double tStart = m_viewStart + (m_viewEnd - m_viewStart) * x / numPixels;
            double tEnd = m_viewStart + (m_viewEnd - m_viewStart) * (x + 1) / numPixels;

            int sStart = std::max(0, static_cast<int>(tStart * m_sampleRate));
            int sEnd = std::min(static_cast<int>(m_samples->size()),
                                static_cast<int>(tEnd * m_sampleRate));

            if (sStart >= sEnd || sStart >= static_cast<int>(m_samples->size())) {
                m_cache[x] = {0.0f, 0.0f};
                continue;
            }

            float mn = (*m_samples)[sStart];
            float mx = (*m_samples)[sStart];
            for (int s = sStart + 1; s < sEnd; ++s) {
                mn = std::min(mn, (*m_samples)[s]);
                mx = std::max(mx, (*m_samples)[s]);
            }
            m_cache[x] = {mn, mx};
        }
    }

    int hitTestBoundary(int x) const {
        if (!m_boundaries)
            return -1;
        for (size_t i = 0; i < m_boundaries->size(); ++i) {
            int bx = timeToX((*m_boundaries)[i].timeSec);
            if (std::abs(x - bx) <= 6)
                return static_cast<int>(i);
        }
        return -1;
    }

    double xToTime(int x) const {
        double viewDuration = m_viewEnd - m_viewStart;
        if (viewDuration <= 0.0 || width() <= 0)
            return m_viewStart;
        return m_viewStart + viewDuration * x / width();
    }

    int timeToX(double time) const {
        double viewDuration = m_viewEnd - m_viewStart;
        if (viewDuration <= 0.0 || width() <= 0)
            return 0;
        return static_cast<int>((time - m_viewStart) / viewDuration * width());
    }

    struct MinMax {
        float min;
        float max;
    };

    ViewportController *m_viewport = nullptr;
    const std::vector<float> *m_samples = nullptr;
    const std::vector<BoundaryInfo> *m_boundaries = nullptr;
    int m_sampleRate = 44100;
    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;
    double m_playhead = -1.0;
    std::vector<MinMax> m_cache;
    ToolMode m_toolMode = ToolMode::Pointer;
    int m_selectedBoundary = -1;
    bool m_dragging = false;
    int m_dragIndex = -1;
    double m_dragOriginalTime = 0.0;
    double m_dragCurrentTime = 0.0;
};

// ─── WaveformPanel ───────────────────────────────────────────────────────────

WaveformPanel::WaveformPanel(QWidget *parent) : QWidget(parent) {
    m_viewport = new ViewportController(this);
    m_playback = new PlaybackController(this);

    buildLayout();
    connectSignals();
}

WaveformPanel::~WaveformPanel() = default;

void WaveformPanel::loadAudio(const QString &path) {
    dstools::audio::AudioDecoder decoder;
    if (!decoder.open(path))
        return;

    auto fmt = decoder.format();
    m_sampleRate = fmt.sampleRate();
    m_channelCount = fmt.channels();

    std::vector<float> allSamples;
    constexpr int kBufSize = 4096;
    std::vector<float> buffer(kBufSize);
    while (true) {
        int read = decoder.read(buffer.data(), 0, kBufSize);
        if (read <= 0)
            break;
        allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
    }
    decoder.close();

    // Store per-channel data and mix to mono
    size_t numFrames = allSamples.size() / m_channelCount;
    m_channelSamples.resize(m_channelCount);
    for (int c = 0; c < m_channelCount; ++c) {
        m_channelSamples[c].resize(numFrames);
        for (size_t i = 0; i < numFrames; ++i)
            m_channelSamples[c][i] = allSamples[i * m_channelCount + c];
    }

    // Always create mono mix
    m_samples.resize(numFrames);
    if (m_channelCount > 1) {
        for (size_t i = 0; i < numFrames; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < m_channelCount; ++c)
                sum += m_channelSamples[c][i];
            m_samples[i] = sum / static_cast<float>(m_channelCount);
        }
    } else {
        m_samples = m_channelSamples[0];
    }

    m_waveformDisplay->setSamples(&m_samples, m_sampleRate);
    m_playback->openFile(path);

    double duration = totalDuration();
    m_viewport->setTotalDuration(duration);
    m_viewport->setViewRange(0.0, duration);
    updateScrollBar();

    if (m_channelCount > 1)
        emit stereoDetected();

    emit audioLoaded();
}

void WaveformPanel::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    m_waveformDisplay->setSamples(&m_samples, m_sampleRate);

    double duration = totalDuration();
    m_viewport->setTotalDuration(duration);
    m_viewport->setViewRange(0.0, duration);
    updateScrollBar();

    emit audioLoaded();
}

void WaveformPanel::setBoundaries(const std::vector<BoundaryInfo> &boundaries) {
    m_boundaries = boundaries;
    m_waveformDisplay->setBoundaries(&m_boundaries);
}

void WaveformPanel::setChannelMode(ChannelMode mode) {
    if (m_channelMode == mode)
        return;
    m_channelMode = mode;
    // For now, always display mono mix. StereoSplit rendering
    // will be implemented when WaveformDisplay supports dual-track painting.
    // The mono data pointer stays the same — downstream always uses monoSamples().
    emit channelModeChanged(mode);
}

double WaveformPanel::totalDuration() const {
    if (m_samples.empty() || m_sampleRate <= 0)
        return 0.0;
    return static_cast<double>(m_samples.size()) / m_sampleRate;
}

void WaveformPanel::scrollToTime(double sec) {
    double viewDuration = m_viewport->duration();
    double halfView = viewDuration / 2.0;
    double totalDur = m_viewport->totalDuration();

    double newStart = sec - halfView;
    if (newStart < 0.0)
        newStart = 0.0;
    if (newStart + viewDuration > totalDur)
        newStart = qMax(0.0, totalDur - viewDuration);

    m_viewport->setViewRange(newStart, newStart + viewDuration);
}

void WaveformPanel::setToolMode(ToolMode mode) {
    m_toolMode = mode;
    m_waveformDisplay->setToolMode(mode);
}

void WaveformPanel::setSelectedBoundary(int index) {
    m_selectedBoundary = index;
    m_waveformDisplay->setSelectedBoundary(index);
}

void WaveformPanel::buildLayout() {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_timeRuler = new TimeRuler(m_viewport, this);
    layout->addWidget(m_timeRuler);

    m_waveformDisplay = new WaveformDisplay(m_viewport, this);
    layout->addWidget(m_waveformDisplay, 1);

    m_hScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_hScrollBar->setMinimum(0);
    m_hScrollBar->setMaximum(0);
    layout->addWidget(m_hScrollBar);
}

void WaveformPanel::connectSignals() {
    // Viewport → sub-widgets
    connect(m_viewport, &ViewportController::viewportChanged, m_timeRuler,
            &TimeRuler::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged, m_waveformDisplay,
            &WaveformDisplay::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged, this,
            [this](const ViewportState &) { updateScrollBar(); });

    // Scrollbar → viewport
    connect(m_hScrollBar, &QScrollBar::valueChanged, this,
            &WaveformPanel::onScrollBarValueChanged);

    // Playhead
    connect(m_playback, &PlaybackController::playheadChanged, m_waveformDisplay,
            &WaveformDisplay::setPlayhead);

    // Waveform interaction
    connect(m_waveformDisplay, &WaveformDisplay::positionClicked, this,
            &WaveformPanel::positionClicked);
    connect(m_waveformDisplay, &WaveformDisplay::boundaryClicked, this,
            &WaveformPanel::boundaryClicked);
    connect(m_waveformDisplay, &WaveformDisplay::boundaryDragged, this,
            &WaveformPanel::boundaryMoved);
    connect(m_waveformDisplay, &WaveformDisplay::boundaryRightClicked, this,
            &WaveformPanel::boundaryRightClicked);

    // Right-click play: find surrounding boundaries and play
    connect(m_waveformDisplay, &WaveformDisplay::rightClickPlay, this,
            [this](double clickTime) {
                double segStart = 0.0;
                double segEnd = totalDuration();

                for (const auto &b : m_boundaries) {
                    if (b.timeSec <= clickTime)
                        segStart = b.timeSec;
                    if (b.timeSec > clickTime) {
                        segEnd = b.timeSec;
                        break;
                    }
                }

                m_playback->playSegment(segStart, segEnd);
                emit segmentPlayRequested(segStart, segEnd);
            });

    // Zoom
    connect(m_waveformDisplay, &WaveformDisplay::zoomRequested, this,
            [this](double atTime, double factor) {
                m_viewport->zoomAt(atTime, factor);
                emit zoomChanged(m_viewport->pixelsPerSecond());
            });
}

void WaveformPanel::updateScrollBar() {
    double duration = m_viewport->totalDuration();
    if (duration <= 0.0) {
        m_hScrollBar->setRange(0, 0);
        return;
    }

    double viewDuration = m_viewport->duration();
    int totalMs = static_cast<int>(duration * 1000.0);
    int viewMs = static_cast<int>(viewDuration * 1000.0);
    int maxVal = qMax(0, totalMs - viewMs);

    QSignalBlocker blocker(m_hScrollBar);
    m_hScrollBar->setRange(0, maxVal);
    m_hScrollBar->setPageStep(viewMs);
    m_hScrollBar->setSingleStep(qMax(1, viewMs / 10));
    m_hScrollBar->setValue(static_cast<int>(m_viewport->startSec() * 1000.0));
}

void WaveformPanel::onScrollBarValueChanged(int value) {
    double startSec = value / 1000.0;
    double viewDuration = m_viewport->duration();
    m_viewport->setViewRange(startSec, startSec + viewDuration);
}

} // namespace waveform
} // namespace dstools

#include "WaveformPanel.moc"
