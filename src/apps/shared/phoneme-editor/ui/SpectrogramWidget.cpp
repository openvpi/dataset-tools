#include "SpectrogramWidget.h"
#include "IBoundaryModel.h"
#include "BoundaryBindingManager.h"
#include "commands/MoveBoundaryCommand.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QResizeEvent>
#include <QUndoStack>

#include <cmath>
#include <algorithm>
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dstools {
namespace phonemelabeler {

SpectrogramWidget::SpectrogramWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent),
    m_viewport(viewport)
{
    setMouseTracking(true);
    setMinimumHeight(80);

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

SpectrogramWidget::~SpectrogramWidget() = default;

void SpectrogramWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    prepareSpectrogramParams();
    rebuildViewImage();
    update();
}

void SpectrogramWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelsPerSecond = state.pixelsPerSecond;
    rebuildViewImage();
    update();
}

std::vector<double> SpectrogramWidget::makeBlackmanHarrisWindow(int N) {
    std::vector<double> w(N);
    for (int n = 0; n < N; ++n) {
        double x = 2.0 * M_PI * n / (N - 1);
        // Blackman-Harris window coefficients: a0, a1, a2, a3
        w[n] = 0.35875 - 0.48829 * std::cos(x) + 0.14128 * std::cos(2.0 * x) - 0.01168 * std::cos(3.0 * x);
    }
    return w;
}

void SpectrogramWidget::prepareSpectrogramParams() {
    m_spectrogramData.clear();
    m_computedFrames.clear();
    m_fftWindow.clear();
    m_numFreqBins = 0;
    m_totalFrames = 0;

    if (m_samples.empty() || m_sampleRate <= 0) return;

    // Scale hop and window size based on sample rate (vLabeler approach)
    double sampleRateRatio = m_sampleRate / kStandardSampleRate;
    m_hopSize = std::max(1, static_cast<int>(std::round(kStandardHopSize * sampleRateRatio)));

    // Window size scales exponentially base 2
    double windowExp = std::log2(kStandardWindowSize) + std::log2(sampleRateRatio);
    m_windowSize = static_cast<int>(std::round(std::pow(2.0, windowExp)));
    // Round to nearest power of 2 for FFT efficiency
    int p = 1;
    while (p < m_windowSize) p <<= 1;
    m_windowSize = p;

    m_fftWindow = makeBlackmanHarrisWindow(m_windowSize);

    int halfWindow = m_windowSize / 2;
    int fftOutSize = m_windowSize / 2 + 1;

    // How many frequency bins to keep (up to maxFrequency)
    int maxBins = static_cast<int>(static_cast<double>(fftOutSize) * kMaxFrequency / m_sampleRate * 2.0);
    maxBins = std::min(maxBins, fftOutSize);
    m_numFreqBins = maxBins;

    // Pad samples
    int totalSamples = static_cast<int>(m_samples.size());
    int paddedSize = totalSamples + m_windowSize; // halfWindow on each side

    // Number of frames
    m_totalFrames = (paddedSize - m_windowSize) / m_hopSize + 1;
    if (m_totalFrames <= 0) {
        m_totalFrames = 0;
        return;
    }

    // Allocate empty spectrogram data (not computed yet)
    m_spectrogramData.resize(m_totalFrames);
    m_computedFrames.resize(m_totalFrames, false);
}

void SpectrogramWidget::computeSpectrogramRange(int frameStart, int frameEnd) {
    if (m_samples.empty() || m_totalFrames <= 0 || m_fftWindow.empty()) return;

    frameStart = std::max(0, frameStart);
    frameEnd = std::min(m_totalFrames, frameEnd);
    if (frameStart >= frameEnd) return;

    int halfWindow = m_windowSize / 2;
    int fftOutSize = m_windowSize / 2 + 1;

    int totalSamples = static_cast<int>(m_samples.size());
    int paddedSize = totalSamples + m_windowSize;

    // Compute window energy for proper normalization
    double windowEnergy = 0.0;
    for (int i = 0; i < m_windowSize; ++i)
        windowEnergy += m_fftWindow[i] * m_fftWindow[i];

    // Allocate FFTW buffers
    double *fftIn = fftw_alloc_real(m_windowSize);
    fftw_complex *fftOut = fftw_alloc_complex(fftOutSize);
    fftw_plan plan = fftw_plan_dft_r2c_1d(m_windowSize, fftIn, fftOut, FFTW_ESTIMATE);

    for (int frame = frameStart; frame < frameEnd; ++frame) {
        if (m_computedFrames[frame]) continue;

        int offset = frame * m_hopSize;

        // Apply window with inline padding
        for (int i = 0; i < m_windowSize; ++i) {
            int srcIdx = offset + i - halfWindow;
            double sample = 0.0;
            if (srcIdx >= 0 && srcIdx < totalSamples)
                sample = m_samples[srcIdx];
            fftIn[i] = sample * m_fftWindow[i];
        }

        // FFT
        fftw_execute(plan);

        // Power spectrum → dB → normalize
        auto &bins = m_spectrogramData[frame];
        bins.resize(m_numFreqBins);

        for (int b = 0; b < m_numFreqBins; ++b) {
            double re = fftOut[b][0];
            double im = fftOut[b][1];
            double power = (re * re + im * im) / windowEnergy;
            if (b > 0 && b < m_windowSize / 2)
                power *= 2.0;

            double dB;
            if (power < 1e-20) {
                dB = kMinIntensityDb;
            } else {
                dB = 10.0 * std::log10(power);
            }

            double norm = (dB - kMinIntensityDb) / (kMaxIntensityDb - kMinIntensityDb);
            bins[b] = static_cast<float>(std::clamp(norm, 0.0, 1.0));
        }

        m_computedFrames[frame] = true;
    }

    fftw_destroy_plan(plan);
    fftw_free(fftIn);
    fftw_free(fftOut);
}

void SpectrogramWidget::ensureSpectrogramRange(int frameStart, int frameEnd) {
    frameStart = std::max(0, frameStart);
    frameEnd = std::min(m_totalFrames, frameEnd);
    if (frameStart >= frameEnd) return;

    // Check if any frames in range need computing
    bool needsCompute = false;
    for (int f = frameStart; f < frameEnd; ++f) {
        if (!m_computedFrames[f]) {
            needsCompute = true;
            break;
        }
    }

    if (needsCompute) {
        computeSpectrogramRange(frameStart, frameEnd);
    }
}

QRgb SpectrogramWidget::intensityToColor(float normalized) const {
    float v = std::clamp(normalized, 0.0f, 1.0f);
    // Square root scaling for perceptually uniform brightness
    v = std::sqrt(v);
    return m_palette.map(v);
}

void SpectrogramWidget::setColorPalette(const SpectrogramColorPalette &palette) {
    m_palette = palette;
    // Force full rebuild of cached image
    m_cachedViewStart = -1.0;
    m_cachedViewEnd = -1.0;
    rebuildViewImage();
    update();
}

void SpectrogramWidget::rebuildViewImage() {
    if (m_totalFrames <= 0 || m_numFreqBins <= 0) {
        m_viewImage = QImage();
        return;
    }

    int w = width();
    int h = height();
    if (w <= 0) w = 1;
    if (h <= 0) h = 200;

    // Check if we can reuse cached image
    if (m_viewImage.width() == w && m_viewImage.height() == h &&
        m_cachedViewStart == m_viewStart && m_cachedViewEnd == m_viewEnd) {
        return;
    }

    double totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;
    int totalFrames = m_totalFrames;

    // Determine the frame range needed for the current view (with margin)
    double viewFrameStartF = (m_viewStart / totalDuration) * totalFrames;
    double viewFrameEndF = (m_viewEnd / totalDuration) * totalFrames;
    int margin = 2;
    int frameStart = std::max(0, static_cast<int>(viewFrameStartF) - margin);
    int frameEnd = std::min(totalFrames, static_cast<int>(std::ceil(viewFrameEndF)) + margin);

    // Lazily compute only the needed frames
    ensureSpectrogramRange(frameStart, frameEnd);

    m_viewImage = QImage(w, h, QImage::Format_RGB32);
    m_viewImage.fill(qRgb(0, 0, 0));

    for (int x = 0; x < w; ++x) {
        double t = m_viewStart + (m_viewEnd - m_viewStart) * (x + 0.5) / w;
        // Map time to frame index
        double frameIdx = (t / totalDuration) * totalFrames;
        int frame0 = static_cast<int>(frameIdx);
        int frame1 = frame0 + 1;
        float frameFrac = static_cast<float>(frameIdx - frame0);
        if (frame0 < 0 || frame0 >= totalFrames) continue;
        if (frame1 >= totalFrames) frame1 = frame0;

        // Skip frames that haven't been computed
        if (!m_computedFrames[frame0]) continue;
        if (!m_computedFrames[frame1]) frame1 = frame0;

        const auto &bins0 = m_spectrogramData[frame0];
        const auto &bins1 = m_spectrogramData[frame1];

        if (bins0.empty() || bins1.empty()) continue;

        for (int y = 0; y < h; ++y) {
            // y=0 is top (high freq), y=h-1 is bottom (low freq)
            float binFrac = static_cast<float>(h - 1 - y) / (h - 1) * (m_numFreqBins - 1);
            int bin = static_cast<int>(binFrac);
            if (bin < 0 || bin >= m_numFreqBins) continue;

            // Bilinear interpolation between bins and frames
            float val0, val1;
            if (bin + 1 < m_numFreqBins) {
                float frac = binFrac - bin;
                val0 = bins0[bin] * (1.0f - frac) + bins0[bin + 1] * frac;
                val1 = bins1[bin] * (1.0f - frac) + bins1[bin + 1] * frac;
            } else {
                val0 = bins0[bin];
                val1 = bins1[bin];
            }
            float val = val0 * (1.0f - frameFrac) + val1 * frameFrac;

            m_viewImage.setPixel(x, y, intensityToColor(val));
        }
    }

    m_cachedViewStart = m_viewStart;
    m_cachedViewEnd = m_viewEnd;
    m_cachedWidth = w;
    m_cachedHeight = h;
}

void SpectrogramWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0));

    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    if (!m_viewImage.isNull()) {
        painter.drawImage(rect(), m_viewImage);
    }

    drawBoundaryOverlay(painter);
}

double SpectrogramWidget::xToTime(int x) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return m_viewStart;
    return m_viewStart + viewDuration * x / width();
}

int SpectrogramWidget::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

void SpectrogramWidget::drawBoundaryOverlay(QPainter &painter) {
    if (!m_boundaryModel) return;

    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount()) return;

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
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

void SpectrogramWidget::updateBoundaryOverlay() {
    update();
}

int SpectrogramWidget::hitTestBoundary(int x) const {
    if (!m_boundaryModel) return -1;
    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount()) return -1;

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        int bx = timeToX(usToSec(m_boundaryModel->boundaryTime(activeTier, b)));
        if (std::abs(x - bx) <= kBoundaryHitWidth / 2) {
            return b;
        }
    }
    return -1;
}

void SpectrogramWidget::startBoundaryDrag(int boundaryIndex, TimePos time) {
    if (!m_boundaryModel) return;
    m_boundaryDragging = true;
    m_draggedBoundary = boundaryIndex;
    m_draggedTier = m_boundaryModel->activeTierIndex();
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

void SpectrogramWidget::updateBoundaryDrag(TimePos currentTime) {
    if (!m_boundaryDragging || !m_boundaryModel) return;

    m_boundaryModel->moveBoundary(m_draggedTier, m_draggedBoundary, currentTime);

    TimePos delta = currentTime - m_boundaryDragStartTime;
    for (size_t i = 0; i < m_dragAligned.size(); ++i) {
        m_boundaryModel->moveBoundary(m_dragAligned[i].tierIndex,
                                 m_dragAligned[i].boundaryIndex,
                                 m_dragAlignedStartTimes[i] + delta);
    }

    emit boundaryDragging(m_draggedTier, m_draggedBoundary, currentTime);
}

void SpectrogramWidget::endBoundaryDrag(TimePos finalTime) {
    if (!m_boundaryDragging || !m_boundaryModel) return;

    int draggedTier = m_draggedTier;
    int draggedBoundary = m_draggedBoundary;

    m_boundaryModel->moveBoundary(m_draggedTier, m_draggedBoundary, m_boundaryDragStartTime);
    for (size_t i = 0; i < m_dragAligned.size(); ++i) {
        m_boundaryModel->moveBoundary(m_dragAligned[i].tierIndex,
                                 m_dragAligned[i].boundaryIndex,
                                 m_dragAlignedStartTimes[i]);
    }

    if (m_undoStack) {
        if (!m_dragAligned.empty() && m_bindingMgr) {
            auto *cmd = m_bindingMgr->createLinkedMoveCommand(
                m_draggedTier, m_draggedBoundary, finalTime, m_boundaryModel);
            m_undoStack->push(cmd);
        } else {
            m_undoStack->push(new MoveBoundaryCommand(
                m_boundaryModel, m_draggedTier, m_draggedBoundary,
                m_boundaryDragStartTime, finalTime));
        }
    } else {
        // No undo stack: apply the move directly
        m_boundaryModel->moveBoundary(m_draggedTier, m_draggedBoundary, finalTime);
        TimePos delta = finalTime - m_boundaryDragStartTime;
        for (size_t i = 0; i < m_dragAligned.size(); ++i) {
            m_boundaryModel->moveBoundary(m_dragAligned[i].tierIndex,
                                     m_dragAligned[i].boundaryIndex,
                                     m_dragAlignedStartTimes[i] + delta);
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

void SpectrogramWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int boundaryIdx = hitTestBoundary(event->pos().x());
        if (boundaryIdx >= 0 && m_boundaryModel) {
            int tier = m_boundaryModel->activeTierIndex();
            TimePos t = m_boundaryModel->boundaryTime(tier, boundaryIdx);
            startBoundaryDrag(boundaryIdx, t);
        } else {
            m_dragging = true;
            m_dragStartPos = event->pos();
            m_dragStartTime = xToTime(event->pos().x());
        }
    }
    QWidget::mousePressEvent(event);
}

void SpectrogramWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_boundaryDragging) {
        TimePos currentTime = secToUs(xToTime(event->pos().x()));
        updateBoundaryDrag(currentTime);
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

void SpectrogramWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_boundaryDragging) {
            TimePos finalTime = secToUs(xToTime(event->pos().x()));
            endBoundaryDrag(finalTime);
        }
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void SpectrogramWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        if (m_viewport) {
            m_viewport->zoomAt(xToTime(static_cast<int>(event->position().x())), factor);
        }
    } else {
        // Plain wheel: switch entries
        int d = (event->angleDelta().y() > 0) ? -1 : 1;
        emit entryScrollRequested(d);
        event->accept();
    }
}

void SpectrogramWidget::resizeEvent(QResizeEvent *event) {
    rebuildViewImage();
    QWidget::resizeEvent(event);
}

void SpectrogramWidget::contextMenuEvent(QContextMenuEvent *event) {
    // ADR-62: Right-click = direct play segment (no context menu)
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

void SpectrogramWidget::findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const {
    outStart = 0.0;
    outEnd = m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;

    if (!m_boundaryModel) return;

    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount()) return;

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
        if (t <= timeSec) outStart = t;
        if (t > timeSec) { outEnd = t; break; }
    }
}

} // namespace phonemelabeler
} // namespace dstools
