#include "SpectrogramWidget.h"

#include "BoundaryDragController.h"
#include "IBoundaryModel.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>

#include <cmath>
#include <algorithm>
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dstools {
namespace phonemelabeler {

SpectrogramWidget::SpectrogramWidget(ViewportController *viewport, QWidget *parent)
    : AudioChartWidget(viewport, parent)
{
    setMinimumHeight(200);
}

SpectrogramWidget::~SpectrogramWidget() = default;

void SpectrogramWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    prepareSpectrogramParams();
    rebuildViewImage();
    update();
}

void SpectrogramWidget::rebuildCache() {
    rebuildViewImage();
}

std::vector<double> SpectrogramWidget::makeBlackmanHarrisWindow(int N) {
    std::vector<double> w(N);
    for (int n = 0; n < N; ++n) {
        double x = 2.0 * M_PI * n / (N - 1);
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

    double sampleRateRatio = m_sampleRate / kStandardSampleRate;
    m_hopSize = std::max(1, static_cast<int>(std::round(kStandardHopSize * sampleRateRatio)));

    double windowExp = std::log2(kStandardWindowSize) + std::log2(sampleRateRatio);
    m_windowSize = static_cast<int>(std::round(std::pow(2.0, windowExp)));
    int p = 1;
    while (p < m_windowSize) p <<= 1;
    m_windowSize = p;

    m_fftWindow = makeBlackmanHarrisWindow(m_windowSize);

    int halfWindow = m_windowSize / 2;
    int fftOutSize = m_windowSize / 2 + 1;

    int maxBins = static_cast<int>(static_cast<double>(fftOutSize) * kMaxFrequency / m_sampleRate * 2.0);
    maxBins = std::min(maxBins, fftOutSize);
    m_numFreqBins = maxBins;

    int totalSamples = static_cast<int>(m_samples.size());

    m_totalFrames = (totalSamples - m_windowSize) / m_hopSize + 1;
    if (m_totalFrames <= 0) {
        m_totalFrames = 0;
        return;
    }

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

    double windowEnergy = 0.0;
    for (int i = 0; i < m_windowSize; ++i)
        windowEnergy += m_fftWindow[i] * m_fftWindow[i];

    double *fftIn = fftw_alloc_real(m_windowSize);
    fftw_complex *fftOut = fftw_alloc_complex(fftOutSize);
    fftw_plan plan = fftw_plan_dft_r2c_1d(m_windowSize, fftIn, fftOut, FFTW_ESTIMATE);

    for (int frame = frameStart; frame < frameEnd; ++frame) {
        if (m_computedFrames[frame]) continue;

        int offset = frame * m_hopSize;

        for (int i = 0; i < m_windowSize; ++i) {
            int srcIdx = offset + i;
            double sample = 0.0;
            if (srcIdx >= 0 && srcIdx < totalSamples)
                sample = m_samples[srcIdx];
            fftIn[i] = sample * m_fftWindow[i];
        }

        fftw_execute(plan);

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
    v = std::sqrt(v);
    return m_palette.map(v);
}

void SpectrogramWidget::setColorPalette(const SpectrogramColorPalette &palette) {
    m_palette = palette;
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

    double viewStart = m_viewStart;
    double viewEnd = m_viewEnd;

    if (m_viewImage.width() == w && m_viewImage.height() == h &&
        m_cachedViewStart == viewStart && m_cachedViewEnd == viewEnd) {
        return;
    }

    double totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;
    int totalFrames = m_totalFrames;

    double viewFrameStartF = (viewStart / totalDuration) * totalFrames;
    double viewFrameEndF = (viewEnd / totalDuration) * totalFrames;
    int margin = 2;
    int frameStart = std::max(0, static_cast<int>(viewFrameStartF) - margin);
    int frameEnd = std::min(totalFrames, static_cast<int>(std::ceil(viewFrameEndF)) + margin);

    ensureSpectrogramRange(frameStart, frameEnd);

    m_viewImage = QImage(w, h, QImage::Format_RGB32);
    m_viewImage.fill(qRgb(0, 0, 0));

    for (int x = 0; x < w; ++x) {
        double t = viewStart + (viewEnd - viewStart) * (x + 0.5) / w;
        double frameIdx = (t / totalDuration) * totalFrames;
        int frame0 = static_cast<int>(frameIdx);
        int frame1 = frame0 + 1;
        float frameFrac = static_cast<float>(frameIdx - frame0);
        if (frame0 < 0 || frame0 >= totalFrames) continue;
        if (frame1 >= totalFrames) frame1 = frame0;

        if (!m_computedFrames[frame0]) continue;
        if (!m_computedFrames[frame1]) frame1 = frame0;

        const auto &bins0 = m_spectrogramData[frame0];
        const auto &bins1 = m_spectrogramData[frame1];

        if (bins0.empty() || bins1.empty()) continue;

        for (int y = 0; y < h; ++y) {
            float binFrac = static_cast<float>(h - 1 - y) / (h - 1) * (m_numFreqBins - 1);
            int bin = static_cast<int>(binFrac);
            if (bin < 0 || bin >= m_numFreqBins) continue;

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

    m_cachedViewStart = viewStart;
    m_cachedViewEnd = viewEnd;
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

void SpectrogramWidget::drawBoundaryOverlay(QPainter &painter) {
    if (!m_boundaryModel) return;

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

void SpectrogramWidget::resizeEvent(QResizeEvent *event) {
    rebuildViewImage();
    AudioChartWidget::resizeEvent(event);
}

void SpectrogramWidget::showEvent(QShowEvent *event) {
    AudioChartWidget::showEvent(event);
    emit visibleStateChanged(true);
}

void SpectrogramWidget::hideEvent(QHideEvent *event) {
    AudioChartWidget::hideEvent(event);
    emit visibleStateChanged(false);
}

} // namespace phonemelabeler
} // namespace dstools