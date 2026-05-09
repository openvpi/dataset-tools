#include "PowerWidget.h"
#include "TextGridDocument.h"
#include "BoundaryDragController.h"
#include "commands/BoundaryCommands.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>
#include <QContextMenuEvent>
#include <QUndoStack>

#include <cmath>
#include <algorithm>

namespace dstools {
namespace phonemelabeler {

PowerWidget::PowerWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent),
    m_viewport(viewport)
{
    setMouseTracking(true);
    setMinimumHeight(80);

    if (m_viewport) {
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
    }
}

PowerWidget::~PowerWidget() = default;

void PowerWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    rebuildPowerCache();
    update();
}

void PowerWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    rebuildPowerCache();
    update();
}

void PowerWidget::setDocument(TextGridDocument *doc) {
    m_document = doc;
    update();
}

void PowerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    drawReferenceLines(painter);
    drawPower(painter);
    drawBoundaryOverlay(painter);
}

void PowerWidget::drawReferenceLines(QPainter & /*painter*/) {
}

void PowerWidget::drawPower(QPainter &painter) {
    if (m_powerCache.empty()) return;

    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int numPixels = std::min(w, static_cast<int>(m_powerCache.size()));
    if (numPixels < 2) return;

    // Compute Y for each pixel
    auto getY = [&](int x) -> float {
        float norm = std::clamp((m_powerCache[x] - kMinPower) / (kMaxPower - kMinPower), 0.0f, 1.0f);
        return h - norm * h;
    };

    // Subsample: take control points every kStep pixels
    constexpr int kStep = 4;
    struct Pt { float x, y; };
    std::vector<Pt> pts;
    for (int x = 0; x < numPixels; x += kStep) {
        pts.push_back({static_cast<float>(x), getY(x)});
    }
    // Always include the last point
    if (pts.empty() || static_cast<int>(pts.back().x) != numPixels - 1) {
        pts.push_back({static_cast<float>(numPixels - 1), getY(numPixels - 1)});
    }

    if (pts.size() < 2) return;

    // Build Catmull-Rom spline converted to cubic Bezier segments
    QPainterPath strokePath;
    strokePath.moveTo(pts[0].x, pts[0].y);

    QPainterPath fillPath;
    fillPath.moveTo(0, h);
    fillPath.lineTo(pts[0].x, pts[0].y);

    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        Pt p0 = (i > 0) ? pts[i - 1] : pts[i];
        Pt p1 = pts[i];
        Pt p2 = pts[i + 1];
        Pt p3 = (i + 2 < pts.size()) ? pts[i + 2] : pts[i + 1];

        // Catmull-Rom to Bezier control points
        float cp1x = p1.x + (p2.x - p0.x) / 6.0f;
        float cp1y = p1.y + (p2.y - p0.y) / 6.0f;
        float cp2x = p2.x - (p3.x - p1.x) / 6.0f;
        float cp2y = p2.y - (p3.y - p1.y) / 6.0f;

        strokePath.cubicTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y);
        fillPath.cubicTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y);
    }

    fillPath.lineTo(numPixels - 1, h);
    fillPath.closeSubpath();

    // Fill under curve
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(128, 255, 128, 60));
    painter.drawPath(fillPath);

    // Stroke the curve
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(128, 255, 128, 200), 1.5));
    painter.drawPath(strokePath);

    painter.setRenderHint(QPainter::Antialiasing, false);
}

void PowerWidget::rebuildPowerCache() {
    if (m_samples.empty() || m_viewEnd <= m_viewStart) {
        m_powerCache.clear();
        return;
    }

    int numPixels = std::max(1, width());
    m_powerCache.resize(numPixels);

    int halfWindow = kWindowSize / 2;

    for (int x = 0; x < numPixels; ++x) {
        // Center time for this pixel
        double tCenter = m_viewStart + (m_viewEnd - m_viewStart) * (x + 0.5) / numPixels;
        int centerSample = static_cast<int>(tCenter * m_sampleRate);

        int wStart = std::max(0, centerSample - halfWindow);
        int wEnd = std::min(static_cast<int>(m_samples.size()), centerSample + halfWindow);

        if (wStart >= wEnd) {
            m_powerCache[x] = kMinPower;
            continue;
        }

        // RMS calculation
        double sumSq = 0.0;
        for (int s = wStart; s < wEnd; ++s) {
            double val = m_samples[s] * kRefValue; // scale to 16-bit range
            sumSq += val * val;
        }
        double rms = std::sqrt(sumSq / (wEnd - wStart));

        // Convert to dB: 20*log10(rms/ref) where ref = 2^15
        float dB;
        if (rms < 1e-10) {
            dB = kMinPower;
        } else {
            dB = static_cast<float>(20.0 * std::log10(rms / kRefValue));
        }
        m_powerCache[x] = std::clamp(dB, kMinPower, kMaxPower);
    }
}

double PowerWidget::xToTime(int x) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return m_viewStart;
    return m_viewStart + viewDuration * x / width();
}

int PowerWidget::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

void PowerWidget::drawBoundaryOverlay(QPainter &painter) {
    if (!m_document) return;

    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return;

    int count = m_document->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_document->boundaryTime(activeTier, b));
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

void PowerWidget::updateBoundaryOverlay() {
    update();
}

int PowerWidget::hitTestBoundary(int x, int *outTier) const {
    if (!m_document) return -1;

    struct Hit {
        int tier;
        int boundary;
        int dist;
        int dragRoom;
    };
    QVector<Hit> hits;

    int tierCount = m_document->tierCount();
    int activeTier = m_document->activeTierIndex();

    for (int t = 0; t < tierCount; ++t) {
        int count = m_document->boundaryCount(t);
        for (int b = 0; b < count; ++b) {
            int bx = timeToX(usToSec(m_document->boundaryTime(t, b)));
            int dist = std::abs(x - bx);
            if (dist <= kBoundaryHitWidth / 2) {
                TimePos pos = m_document->boundaryTime(t, b);
                TimePos leftClamp = (b > 0) ? m_document->boundaryTime(t, b - 1) : 0;
                TimePos rightClamp = (b + 1 < count) ? m_document->boundaryTime(t, b + 1) : INT64_MAX;
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

void PowerWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int hitTier = -1;
        int boundaryIdx = hitTestBoundary(event->pos().x(), &hitTier);
        if (boundaryIdx >= 0 && hitTier >= 0 && m_document && m_dragController) {
            m_dragController->startDrag(hitTier, boundaryIdx, m_document);
            setCursor(Qt::SizeHorCursor);
        } else {
            m_dragging = true;
            m_dragStartPos = event->pos();
            m_dragStartTime = xToTime(event->pos().x());
        }
    }
    QWidget::mousePressEvent(event);
}

void PowerWidget::mouseMoveEvent(QMouseEvent *event) {
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

void PowerWidget::mouseReleaseEvent(QMouseEvent *event) {
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

void PowerWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        if (m_viewport) {
            m_viewport->zoomAt(xToTime(static_cast<int>(event->position().x())), factor);
        }
        event->accept();
    } else {
        if (m_viewport) {
            double scrollSec = (event->angleDelta().y() > 0) ? -0.5 : 0.5;
            m_viewport->scrollBy(scrollSec);
        }
        event->accept();
    }
}

void PowerWidget::contextMenuEvent(QContextMenuEvent *event) {
    if (m_samples.empty() || !m_playWidget || !m_document) {
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

void PowerWidget::findSurroundingBoundaries(double time, double &start, double &end) const {
    if (!m_document) {
        start = 0.0;
        end = m_viewEnd;
        return;
    }

    int tier = m_document->activeTierIndex();
    int count = m_document->boundaryCount(tier);

    start = 0.0;
    end = usToSec(m_document->totalDuration());

    for (int i = 0; i < count; ++i) {
        double bTime = usToSec(m_document->boundaryTime(tier, i));
        if (bTime <= time)
            start = bTime;
        if (bTime >= time && bTime < end) {
            end = bTime;
            break;
        }
    }
}

void PowerWidget::resizeEvent(QResizeEvent *event) {
    rebuildPowerCache();
    QWidget::resizeEvent(event);
}

void PowerWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    emit visibleStateChanged(true);
}

void PowerWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    emit visibleStateChanged(false);
}

} // namespace phonemelabeler
} // namespace dstools
