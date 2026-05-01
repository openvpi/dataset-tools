#include "WaveformWidget.h"
#include "TextGridDocument.h"
#include "BoundaryBindingManager.h"
#include "commands/MoveBoundaryCommand.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QUndoStack>
#include <QDebug>
#include <QMenu>
#include <QContextMenuEvent>

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
        connect(m_viewport, &ViewportController::viewportChanged,
                this, [this](const ViewportState &state) {
                    setViewport(state);
                });
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
        m_pixelsPerSecond = m_viewport->state().pixelsPerSecond;
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

void WaveformWidget::setDocument(TextGridDocument *doc) {
    m_document = doc;
}

void WaveformWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelsPerSecond = state.pixelsPerSecond;
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
        int yMin = centerY - static_cast<int>(mm.max * centerY);
        int yMax = centerY - static_cast<int>(mm.min * centerY);
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

void WaveformWidget::drawBoundaryOverlay(QPainter &painter) {
    if (!m_boundaryOverlayEnabled || !m_document) return;

    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return;

    int count = m_document->boundaryCount(activeTier);

    for (int b = 0; b < count; ++b) {
        double t = m_document->boundaryTime(activeTier, b);
        int x = timeToX(t);
        if (x < 0 || x > width()) continue;

        if (m_boundaryDragging && b == m_draggedBoundary) {
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
    if (m_samples.empty() || m_pixelsPerSecond <= 0.0) {
        m_minMaxCache.clear();
        return;
    }

    int numPixels = qMax(1, width());
    m_minMaxCache.resize(numPixels);
    m_cachePixelsPerSecond = m_pixelsPerSecond;

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
        // Check if clicking on a boundary first
        int boundaryIdx = hitTestBoundary(event->pos().x());
        if (boundaryIdx >= 0 && m_document) {
            int tier = m_document->activeTierIndex();
            double t = m_document->boundaryTime(tier, boundaryIdx);
            startBoundaryDrag(boundaryIdx, t);
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
    if (m_boundaryDragging) {
        double currentTime = xToTime(event->pos().x());
        updateBoundaryDrag(currentTime);
    } else if (m_dragging) {
        // Drag to scroll
        double deltaSec = xToTime(event->pos().x()) - m_dragStartTime;
        if (m_viewport) {
            m_viewport->scrollBy(-deltaSec);
        }
    } else {
        // Hover detection for boundaries
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
        if (m_boundaryDragging) {
            double finalTime = xToTime(event->pos().x());
            endBoundaryDrag(finalTime);
        }
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

int WaveformWidget::hitTestBoundary(int x) const {
    if (!m_document || !m_boundaryOverlayEnabled) return -1;
    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return -1;

    int count = m_document->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        int bx = timeToX(m_document->boundaryTime(activeTier, b));
        if (std::abs(x - bx) <= kBoundaryHitWidth / 2) {
            return b;
        }
    }
    return -1;
}

void WaveformWidget::startBoundaryDrag(int boundaryIndex, double time) {
    if (!m_document) return;
    m_boundaryDragging = true;
    m_draggedBoundary = boundaryIndex;
    m_draggedTier = m_document->activeTierIndex();
    m_boundaryDragStartTime = time;

    if (m_bindingMgr && m_bindingMgr->isEnabled()) {
        m_dragAligned = m_bindingMgr->findAlignedBoundaries(m_draggedTier, boundaryIndex);
        m_dragAlignedStartTimes.clear();
        for (const auto &ab : m_dragAligned) {
            m_dragAlignedStartTimes.push_back(ab.time);
        }
    } else {
        m_dragAligned.clear();
        m_dragAlignedStartTimes.clear();
    }

    setCursor(Qt::SizeHorCursor);
    emit boundaryDragStarted(m_draggedTier, boundaryIndex);
}

void WaveformWidget::updateBoundaryDrag(double currentTime) {
    if (!m_boundaryDragging || !m_document) return;

    m_document->moveBoundary(m_draggedTier, m_draggedBoundary, currentTime);

    double delta = currentTime - m_boundaryDragStartTime;
    for (size_t i = 0; i < m_dragAligned.size(); ++i) {
        m_document->moveBoundary(m_dragAligned[i].tierIndex,
                                 m_dragAligned[i].boundaryIndex,
                                 m_dragAlignedStartTimes[i] + delta);
    }

    emit boundaryDragging(m_draggedTier, m_draggedBoundary, currentTime);
}

void WaveformWidget::endBoundaryDrag(double finalTime) {
    if (!m_boundaryDragging || !m_document) return;

    int draggedTier = m_draggedTier;
    int draggedBoundary = m_draggedBoundary;

    // Restore to start state, then push undo command
    m_document->moveBoundary(m_draggedTier, m_draggedBoundary, m_boundaryDragStartTime);
    for (size_t i = 0; i < m_dragAligned.size(); ++i) {
        m_document->moveBoundary(m_dragAligned[i].tierIndex,
                                 m_dragAligned[i].boundaryIndex,
                                 m_dragAlignedStartTimes[i]);
    }

    if (m_undoStack) {
        if (!m_dragAligned.empty() && m_bindingMgr) {
            auto *cmd = m_bindingMgr->createLinkedMoveCommand(
                m_draggedTier, m_draggedBoundary, finalTime, m_document);
            m_undoStack->push(cmd);
        } else {
            m_undoStack->push(new MoveBoundaryCommand(
                m_document, m_draggedTier, m_draggedBoundary,
                m_boundaryDragStartTime, finalTime));
        }
    }

    m_boundaryDragging = false;
    m_draggedBoundary = -1;
    m_draggedTier = -1;
    m_dragAligned.clear();
    m_dragAlignedStartTimes.clear();
    unsetCursor();
    emit boundaryDragFinished(draggedTier, draggedBoundary, finalTime);
    update();
}

void WaveformWidget::findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const {
    outStart = 0.0;
    outEnd = m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;

    if (!m_document) return;

    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return;

    int count = m_document->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = m_document->boundaryTime(activeTier, b);
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
    double clickTime = xToTime(event->pos().x());

    QMenu menu(this);

    double segStart, segEnd;
    findSurroundingBoundaries(clickTime, segStart, segEnd);

    QString label = tr("Play segment (%1s - %2s)")
        .arg(segStart, 0, 'f', 3)
        .arg(segEnd, 0, 'f', 3);

    QAction *playAction = menu.addAction(label);
    connect(playAction, &QAction::triggered, this, [this, segStart, segEnd]() {
        if (m_playWidget) {
            m_playWidget->setPlayRange(segStart, segEnd);
            m_playWidget->seek(segStart);
            m_playWidget->setPlaying(true);
        }
    });

    menu.exec(event->globalPos());
}

void WaveformWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        if (m_viewport) {
            m_viewport->zoomAt(xToTime(event->position().x()), factor);
        }
    } else {
        // Plain wheel: switch entries
        int d = (event->angleDelta().y() > 0) ? -1 : 1;
        emit entryScrollRequested(d);
        event->accept();
    }
}

void WaveformWidget::resizeEvent(QResizeEvent *event) {
    rebuildMinMaxCache();
    QWidget::resizeEvent(event);
}

} // namespace phonemelabeler
} // namespace dstools
