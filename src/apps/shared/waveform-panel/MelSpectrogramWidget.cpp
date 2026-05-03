#include "MelSpectrogramWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward-declare FFTW3 if available; otherwise provide stub
#if __has_include(<fftw3.h>)
#include <fftw3.h>
#define HAS_FFTW 1
#else
#define HAS_FFTW 0
#endif

namespace dstools {
namespace waveform {

MelSpectrogramWidget::MelSpectrogramWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent), m_viewport(viewport) {
    setMinimumHeight(60);
    setMaximumHeight(200);

    if (m_viewport) {
        connect(m_viewport, &ViewportController::viewportChanged, this,
                [this](const ViewportState &state) { setViewport(state); });
    }
}

MelSpectrogramWidget::~MelSpectrogramWidget() = default;

void MelSpectrogramWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    computeMelSpectrogram();
    rebuildViewImage();
    update();
}

void MelSpectrogramWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelsPerSecond = state.pixelsPerSecond;
    rebuildViewImage();
    update();
}

void MelSpectrogramWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(20, 20, 20));

    if (m_melData.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No spectrogram data"));
        return;
    }

    if (!m_viewImage.isNull())
        painter.drawImage(rect(), m_viewImage);
}

void MelSpectrogramWidget::resizeEvent(QResizeEvent * /*event*/) {
    rebuildViewImage();
}

void MelSpectrogramWidget::computeMelSpectrogram() {
    m_melData.clear();
    if (m_samples.empty())
        return;

#if HAS_FFTW
    int N = m_windowSize;
    int hop = m_hopSize;
    int numFrames = static_cast<int>((m_samples.size() - N) / hop) + 1;
    if (numFrames <= 0)
        return;

    auto window = makeHannWindow(N);
    auto filterbank = melFilterbank(m_numMelBins, N, m_sampleRate, 0.0, kMaxFreq);

    // Allocate FFTW
    double *fftIn = fftw_alloc_real(N);
    fftw_complex *fftOut = fftw_alloc_complex(N / 2 + 1);
    fftw_plan plan = fftw_plan_dft_r2c_1d(N, fftIn, fftOut, FFTW_ESTIMATE);

    int numBins = N / 2 + 1;
    m_melData.resize(numFrames);

    for (int frame = 0; frame < numFrames; ++frame) {
        int offset = frame * hop;

        // Window and copy
        for (int i = 0; i < N; ++i)
            fftIn[i] = m_samples[offset + i] * window[i];

        fftw_execute(plan);

        // Power spectrum
        std::vector<double> power(numBins);
        for (int i = 0; i < numBins; ++i)
            power[i] = fftOut[i][0] * fftOut[i][0] + fftOut[i][1] * fftOut[i][1];

        // Apply mel filterbank
        m_melData[frame].resize(m_numMelBins);
        for (int m = 0; m < m_numMelBins; ++m) {
            double energy = 0.0;
            for (int k = 0; k < numBins && k < static_cast<int>(filterbank[m].size()); ++k)
                energy += filterbank[m][k] * power[k];

            // Log scale, normalize
            double db = 10.0 * std::log10(std::max(energy, 1e-10));
            float normalized = static_cast<float>((db - kMinDb) / (kMaxDb - kMinDb));
            m_melData[frame][m] = std::clamp(normalized, 0.0f, 1.0f);
        }
    }

    fftw_destroy_plan(plan);
    fftw_free(fftIn);
    fftw_free(fftOut);
#endif
}

void MelSpectrogramWidget::rebuildViewImage() {
    int w = width();
    int h = height();
    if (w <= 0 || h <= 0 || m_melData.empty()) {
        m_viewImage = QImage();
        return;
    }

    // Check if cache is valid
    if (m_cachedViewStart == m_viewStart && m_cachedViewEnd == m_viewEnd &&
        m_cachedWidth == w && m_cachedHeight == h) {
        return;
    }

    m_viewImage = QImage(w, h, QImage::Format_RGB32);
    m_viewImage.fill(QColor(20, 20, 20));

    double totalDuration =
        m_samples.empty() ? 0.0 : static_cast<double>(m_samples.size()) / m_sampleRate;
    if (totalDuration <= 0.0)
        return;

    int totalFrames = static_cast<int>(m_melData.size());

    for (int x = 0; x < w; ++x) {
        double t = m_viewStart + (m_viewEnd - m_viewStart) * x / w;
        int frame = static_cast<int>(t / totalDuration * totalFrames);
        frame = std::clamp(frame, 0, totalFrames - 1);

        for (int y = 0; y < h; ++y) {
            // Map y to mel bin (bottom = low freq, top = high freq)
            int melBin = static_cast<int>((h - 1 - y) * m_numMelBins / h);
            melBin = std::clamp(melBin, 0, m_numMelBins - 1);

            float val = m_melData[frame][melBin];
            // Orange-yellow color map
            int r = static_cast<int>(std::clamp(val * 2.0f, 0.0f, 1.0f) * 255);
            int g = static_cast<int>(std::clamp(val * 1.5f - 0.25f, 0.0f, 1.0f) * 255);
            int b = static_cast<int>(std::clamp(val * 0.5f - 0.3f, 0.0f, 1.0f) * 60);
            m_viewImage.setPixel(x, y, qRgb(r, g, b));
        }
    }

    m_cachedViewStart = m_viewStart;
    m_cachedViewEnd = m_viewEnd;
    m_cachedWidth = w;
    m_cachedHeight = h;
}

std::vector<double> MelSpectrogramWidget::makeHannWindow(int N) {
    std::vector<double> w(N);
    for (int i = 0; i < N; ++i)
        w[i] = 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (N - 1)));
    return w;
}

std::vector<std::vector<double>>
MelSpectrogramWidget::melFilterbank(int numFilters, int fftSize, int sampleRate,
                                     double fMin, double fMax) {
    // Mel scale conversion
    auto hzToMel = [](double hz) { return 2595.0 * std::log10(1.0 + hz / 700.0); };
    auto melToHz = [](double mel) { return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0); };

    double melMin = hzToMel(fMin);
    double melMax = hzToMel(fMax);
    int numBins = fftSize / 2 + 1;

    // Create equally spaced mel points
    std::vector<double> melPoints(numFilters + 2);
    for (int i = 0; i < numFilters + 2; ++i)
        melPoints[i] = melMin + (melMax - melMin) * i / (numFilters + 1);

    // Convert to frequency bin indices
    std::vector<int> binIndices(numFilters + 2);
    for (int i = 0; i < numFilters + 2; ++i) {
        double hz = melToHz(melPoints[i]);
        binIndices[i] = static_cast<int>(std::round(hz * fftSize / sampleRate));
        binIndices[i] = std::clamp(binIndices[i], 0, numBins - 1);
    }

    // Create triangular filters
    std::vector<std::vector<double>> filters(numFilters, std::vector<double>(numBins, 0.0));
    for (int m = 0; m < numFilters; ++m) {
        int left = binIndices[m];
        int center = binIndices[m + 1];
        int right = binIndices[m + 2];

        for (int k = left; k <= center && k < numBins; ++k) {
            if (center > left)
                filters[m][k] = static_cast<double>(k - left) / (center - left);
        }
        for (int k = center; k <= right && k < numBins; ++k) {
            if (right > center)
                filters[m][k] = static_cast<double>(right - k) / (right - center);
        }
    }

    return filters;
}

} // namespace waveform
} // namespace dstools
