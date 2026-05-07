#include "WaveformWidget.h"
#include "IBoundaryModel.h"
#include "BoundaryDragController.h"
#include "commands/BoundaryCommands.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QUndoStack>
#include <QDebug>
#include <QContextMenuEvent>

#include <cmath>
#include <algorithm>

// Audio loading - use dstools-audio (FFmpeg + AudioDecoder)
#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

namespace dstools {
namespace phonemelabeler {

namespace {
    constexpr int kDefaultBufferSize = 4096;
} // namespace

WaveformWidget::WaveformWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent),
    m_viewport(viewport)
{
    setMouseTracking(true);
    setMinimumHeight(150);

    if (m_viewport) {
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
    }
}

WaveformWidget::~WaveformWidget() = default;

void WaveformWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    rebuildMinMaxCache();
    update();
}

void WaveformWidget::loadAudio(const QString &path) {
    dstools::audio::AudioDecoder decoder;
    if (!decoder.open(path)) {
        qWarning() << "Failed to open audio file:" << path;
        return;
    }

    auto fmt = decoder.format();
    m_sampleRate = fmt.sampleRate();

    // Read all samples
    std::vector<float> allSamples;
    const int bufferSize = kDefaultBufferSize;
    std::vector<float> buffer(bufferSize);

    while (true) {
        int read = decoder.read(buffer.data(), 0, bufferSize);
        if (read <= 0) break;
        allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
    }
    decoder.close();

    // Convert to mono if multi-channel
    int channels = fmt.channels();
    if (channels > 1) {
        m_samples.resize(allSamples.size() / channels);
        for (size_t i = 0; i < m_samples.size(); ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c) {
                sum += allSamples[i * channels + c];
            }
            m_samples[i] = sum / channels;
        }
    } else {
        m_samples = std::move(allSamples);
    }

    rebuildMinMaxCache();
    update();
}

void WaveformWidget::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
}

void WaveformWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    rebuildMinMaxCache();
    update();
}

void WaveformWidget::setPlayhead(double sec) {
    m_playhead = sec;
    update();
}

void WaveformWidget::onPlayheadChanged(double sec) {
    setPlayhead(sec);
}

void WaveformWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    drawWaveform(painter);
    drawDbAxis(painter);
    drawPlayCursor(painter);
}

void WaveformWidget::drawWaveform(QPainter &painter) {
    if (m_samples.empty()) return;

    int w = width();
    int h = height();
    int centerY = h / 2;

    // Draw center line
    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawLine(0, centerY, w, centerY);

    // Use cached min/max
    if (m_minMaxCache.empty()) {
        rebuildMinMaxCache();
    }

    painter.setPen(QPen(QColor(100, 180, 255), 1));

    int numPixels = static_cast<int>(m_minMaxCache.size());
    for (int x = 0; x < w && x < numPixels; ++x) {
        const auto &mm = m_minMaxCache[x];
        float scaledMax = std::clamp(mm.max * static_cast<float>(m_amplitudeScale), -1.0f, 1.0f);
        float scaledMin = std::clamp(mm.min * static_cast<float>(m_amplitudeScale), -1.0f, 1.0f);
        int yMin = centerY - static_cast<int>(scaledMax * centerY);
        int yMax = centerY - static_cast<int>(scaledMin * centerY);
        if (yMin > yMax) std::swap(yMin, yMax);
        painter.drawLine(x, yMin, x, yMax);
    }
}

void WaveformWidget::drawPlayCursor(QPainter &painter) {
    if (m_playhead < 0.0) return;

    int x = timeToX(m_playhead);
    if (x < 0 || x > width()) return;

    painter.setPen(QPen(QColor(255, 100, 100), 2));
    painter.drawLine(x, 0, x, height());
}

void WaveformWidget::drawDbAxis(QPainter &painter) {
    int h = height();
    int centerY = h / 2;

    QFont font = painter.font();
    font.setPixelSize(9);
    painter.setFont(font);
    painter.setPen(QColor(100, 100, 120));

    static const float dbValues[] = {0.0f, -6.0f, -12.0f, -24.0f};

    for (float db : dbValues) {
        float linear = std::pow(10.0f, db / 20.0f);
        float scaled = linear * static_cast<float>(m_amplitudeScale);
        if (scaled > 1.0f) continue;

        int yTop = centerY - static_cast<int>(scaled * centerY);
        int yBot = centerY + static_cast<int>(scaled * centerY);

        painter.setPen(QColor(100, 100, 120));
        QString label = (db == 0.0f) ? QStringLiteral("0dB") : QString::number(static_cast<int>(db)) + "dB";
        painter.drawText(2, yTop - 1, label);
    }
}

void WaveformWidget::drawBoundaryOverlay(QPainter &painter) {
    if (!m_boundaryOverlayEnabled || !m_boundaryModel) return;

    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount()) return;

    int count = m_boundaryModel->boundaryCount(activeTier);

    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
        int x = timeToX(t);
        if (x < 0 || x > width()) continue;

        if (m_dragController && m_dragController->isDragging() && b == m_dragController->draggedBoundary()) {
            painter.setPen(QPen(QColor(255, 200, 100), 2));
        } else if (b == m_hoveredBoundary) {
            painter.setPen(QPen(QColor(255, 255, 255), 2));
        } else {
            painter.setPen(QPen(QColor(180, 180, 200, 180), 1, Qt::SolidLine));
        }
        painter.drawLine(x, 0, x, height());
    }
}

void WaveformWidget::setBoundaryOverlayEnabled(bool enabled) {
    m_boundaryOverlayEnabled = enabled;
    update();
}

void WaveformWidget::updateBoundaryOverlay() {
    update();
}

void WaveformWidget::rebuildMinMaxCache() {
    if (m_samples.empty() || m_viewEnd <= m_viewStart) {
        m_minMaxCache.clear();
        return;
    }

    int numPixels = qMax(1, width());
    m_minMaxCache.resize(numPixels);
    m_cacheResolution = (m_viewEnd - m_viewStart);

    for (int x = 0; x < numPixels; ++x) {
        double tStart = m_viewStart + (m_viewEnd - m_viewStart) * x / numPixels;
        double tEnd = m_viewStart + (m_viewEnd - m_viewStart) * (x + 1) / numPixels;

        int sStart = std::max(0, static_cast<int>(tStart * m_sampleRate));
        int sEnd = std::min(static_cast<int>(m_samples.size()),
                            static_cast<int>(tEnd * m_sampleRate));

        if (sStart >= sEnd || sStart >= static_cast<int>(m_samples.size())) {
            m_minMaxCache[x] = {0.0f, 0.0f};
            continue;
        }

        float mn = m_samples[sStart];
        float mx = m_samples[sStart];
        for (int s = sStart + 1; s < sEnd; ++s) {
            mn = std::min(mn, m_samples[s]);
            mx = std::max(mx, m_samples[s]);
        }

        m_minMaxCache[x] = {mn, mx};
    }
}

double WaveformWidget::xToTime(int x) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return m_viewStart;
    return m_viewStart + viewDuration * x / width();
}

int WaveformWidget::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

void WaveformWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int hitTier = -1;
        int boundaryIdx = hitTestBoundary(event->pos().x(), &hitTier);
        if (boundaryIdx >= 0 && hitTier >= 0 && m_boundaryModel && m_dragController) {
            m_dragController->startDrag(hitTier, boundaryIdx, m_boundaryModel);
            setCursor(Qt::SizeHorCursor);
        } else {
            m_dragging = true;
            m_dragStartPos = event->pos();
            m_dragStartTime = xToTime(event->pos().x());
            emit positionClicked(m_dragStartTime);
        }
    }
    QWidget::mousePressEvent(event);
}

void WaveformWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragController && m_dragController->isDragging()) {
        TimePos currentTime = secToUs(xToTime(event->pos().x()));
        m_dragController->updateDrag(currentTime);
    } else if (m_dragging) {
        double deltaSec = xToTime(event->pos().x()) - m_dragStartTime;
        if (m_viewport) {
            m_viewport->scrollBy(-deltaSec);
        }
    } else {
        int boundaryIdx = hitTestBoundary(event->pos().x());
        if (boundaryIdx != m_hoveredBoundary) {
            m_hoveredBoundary = boundaryIdx;
            setCursor(boundaryIdx >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
            emit hoveredBoundaryChanged(boundaryIdx);
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_dragController && m_dragController->isDragging()) {
            TimePos finalTime = secToUs(xToTime(event->pos().x()));
            m_dragController->endDrag(finalTime, m_undoStack);
            unsetCursor();
        }
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

int WaveformWidget::hitTestBoundary(int x, int *outTier) const {
    if (!m_boundaryModel) return -1;

    struct Hit {
        int tier;
        int boundary;
        int dist;
        int dragRoom;
    };
    QVector<Hit> hits;

    int tierCount = m_boundaryModel->tierCount();
    int activeTier = m_boundaryModel->activeTierIndex();

    for (int t = 0; t < tierCount; ++t) {
        int count = m_boundaryModel->boundaryCount(t);
        for (int b = 0; b < count; ++b) {
            int bx = timeToX(usToSec(m_boundaryModel->boundaryTime(t, b)));
            int dist = std::abs(x - bx);
            if (dist <= kBoundaryHitWidth / 2) {
                TimePos pos = m_boundaryModel->boundaryTime(t, b);
                TimePos leftClamp = (b > 0) ? m_boundaryModel->boundaryTime(t, b - 1) : 0;
                TimePos rightClamp = (b + 1 < count) ? m_boundaryModel->boundaryTime(t, b + 1) : INT64_MAX;
                int room = static_cast<int>(std::min(pos - leftClamp, rightClamp - pos));

                hits.push_back({t, b, dist, room});
            }
        }
    }

    if (hits.isEmpty()) {
        if (outTier) *outTier = -1;
        return -1;
    }

    int bestIdx = 0;
    for (int i = 1; i < hits.size(); ++i) {
        const auto &cur = hits[i];
        const auto &best = hits[bestIdx];

        bool curActive = (cur.tier == activeTier);
        bool bestActive = (best.tier == activeTier);

        if (curActive != bestActive) {
            if (curActive) bestIdx = i;
            continue;
        }

        if (cur.dist != best.dist) {
            if (cur.dist < best.dist) bestIdx = i;
            continue;
        }

        if (cur.dragRoom != best.dragRoom) {
            if (cur.dragRoom > best.dragRoom) bestIdx = i;
            continue;
        }

        if (cur.boundary < best.boundary) bestIdx = i;
    }

    if (outTier) *outTier = hits[bestIdx].tier;
    return hits[bestIdx].boundary;
}

void WaveformWidget::findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const {
    outStart = 0.0;
    outEnd = m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;

    if (!m_boundaryModel) return;

    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount()) return;

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
        if (t <= timeSec) {
            outStart = t;
        }
        if (t > timeSec) {
            outEnd = t;
            break;
        }
    }
}

void WaveformWidget::contextMenuEvent(QContextMenuEvent *event) {
    // ADR-62: Right-click = direct play segment (no context menu)
    if (m_samples.empty() || !m_playWidget) {
        event->accept();
        return;
    }

    double clickTime = xToTime(event->pos().x());

    double segStart, segEnd;
    findSurroundingBoundaries(clickTime, segStart, segEnd);

    if (m_playWidget) {
        m_playWidget->setPlayRange(segStart, segEnd);
        m_playWidget->seek(segStart);
        m_playWidget->setPlaying(true);
    }

    event->accept();
}

void WaveformWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        if (m_viewport) {
            m_viewport->zoomAt(xToTime(static_cast<int>(event->position().x())), factor);
        }
        event->accept();
    } else if (event->modifiers() & Qt::ShiftModifier) {
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        m_amplitudeScale = std::clamp(m_amplitudeScale * factor, 0.1, 20.0);
        update();
        event->accept();
    } else {
        if (m_viewport) {
            double scrollSec = (event->angleDelta().y() > 0) ? -0.5 : 0.5;
            m_viewport->scrollBy(scrollSec);
        }
        event->accept();
    }
}

void WaveformWidget::resizeEvent(QResizeEvent *event) {
    rebuildMinMaxCache();
    QWidget::resizeEvent(event);
}

} // namespace phonemelabeler
} // namespace dstools
