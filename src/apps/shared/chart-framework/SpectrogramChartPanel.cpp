#include "SpectrogramChartPanel.h"

#include "ChartConfigRegistry.h"

#include <QPainter>
#include <QResizeEvent>

#include <cmath>
#include <cstring>
#include <algorithm>
#include <fftw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace dstools {

using namespace dsfw;

SpectrogramChartPanel::SpectrogramChartPanel(ViewportController *viewport, QWidget *parent)
    : ChartPanelBase(QStringLiteral("spectrogram"), viewport, parent)
{
    setMinimumHeight(200);
}

void SpectrogramChartPanel::registerChartConfig() {
    static bool registered = false;
    if (registered) return;
    registered = true;

    ChartConfigDescriptor desc;
    desc.chartId = QStringLiteral("spectrogram");
    desc.displayName = QStringLiteral("频谱图");
    desc.params = {
        {QStringLiteral("hopSize"), QStringLiteral("Hop Size"),
         ParamType::Int, QVariant(256), QVariant(64), QVariant(1024),
         QStringLiteral(" samples"), 0,
         QStringLiteral("STFT 帧移大小。越小时间分辨率越高，计算量越大。")},
        {QStringLiteral("windowSize"), QStringLiteral("窗口大小"),
         ParamType::Int, QVariant(2048), QVariant(256), QVariant(8192),
         QStringLiteral(" samples"), 0,
         QStringLiteral("STFT 窗口大小。越大频率分辨率越高，时间分辨率越低。")},
        {QStringLiteral("minDb"), QStringLiteral("最小强度(dB)"),
         ParamType::Double, QVariant(-80.0), QVariant(-200.0), QVariant(0.0),
         QStringLiteral(" dB"), 1,
         QStringLiteral("频谱图动态范围下限。低于此值的强度映射为黑色。")},
        {QStringLiteral("maxDb"), QStringLiteral("最大强度(dB)"),
         ParamType::Double, QVariant(0.0), QVariant(-60.0), QVariant(20.0),
         QStringLiteral(" dB"), 1,
         QStringLiteral("频谱图动态范围上限。高于此值的强度映射为最亮色。")},
    };
    ChartConfigRegistry::instance().registerChart(desc);
}

void SpectrogramChartPanel::loadConfigParams() {
    auto &reg = ChartConfigRegistry::instance();
    m_configHopSize = reg.value(QStringLiteral("spectrogram"), QStringLiteral("hopSize")).toInt();
    m_configWindowSize = reg.value(QStringLiteral("spectrogram"), QStringLiteral("windowSize")).toInt();
    m_configMinDb = reg.value(QStringLiteral("spectrogram"), QStringLiteral("minDb")).toDouble();
    m_configMaxDb = reg.value(QStringLiteral("spectrogram"), QStringLiteral("maxDb")).toDouble();
}

void SpectrogramChartPanel::setColorPalette(const SpectrogramColorPalette &palette) {
    m_palette = palette;
    m_viewImage = QImage();
    m_pendingRegion = RegionUpdate{};
    m_cacheDirty = true;
    update();
}

void SpectrogramChartPanel::onAudioDataChanged() {
    prepareSpectrogramParams();
    m_pendingRegion = RegionUpdate{};
}

void SpectrogramChartPanel::drawContent(QPainter &painter, const ChartCoordinate &coord) {
    Q_UNUSED(coord)
    if (m_samples.empty()) {
        drawEmptyState(painter, tr("No audio loaded"));
        return;
    }

    int yaw = yAxisWidth();

    if (!m_viewImage.isNull()) {
        painter.drawImage(yaw, 0, m_viewImage);
    }
}

void SpectrogramChartPanel::paintYAxisContent(QPainter &painter, const QRect &chartRect) {
    int labelW = chartRect.width();
    int h = height();

    painter.save();
    painter.setPen(QColor(160, 160, 180));
    QFont font(QStringLiteral("Arial"), 8);
    painter.setFont(font);

    double maxFreq = m_sampleRate / 2.0;
    double ticks[] = {1000.0, 2000.0, 4000.0, 8000.0, 16000.0, 22050.0};

    for (double freq : ticks) {
        if (freq > maxFreq) continue;
        double frac = freq / maxFreq;
        int y = static_cast<int>(h - 1 - frac * (h - 1));

        const char *label;
        if (freq >= 1000.0) {
            label = freq >= 10000.0 ? "16k" : freq >= 8000.0 ? "8k" : freq >= 4000.0 ? "4k" : "2k";
        } else {
            label = "1k";
        }
        if (freq >= 22000.0) label = "Nyq";

        int textTop = std::clamp(y - 8, 0, h - 16);
        painter.drawText(QRect(0, textTop, labelW - 4, 16),
                         Qt::AlignRight | Qt::AlignVCenter,
                         QString::fromUtf8(label));
    }

    painter.drawLine(labelW - 1, 0, labelW - 1, h - 1);
    painter.restore();
}

void SpectrogramChartPanel::rebuildCache(const RegionUpdate &region) {
    if (region.fullRebuild) {
        rebuildViewImage();
    } else {
        rebuildViewImagePartial(region);
    }
}

std::vector<double> SpectrogramChartPanel::makeBlackmanHarrisWindow(int N) {
    std::vector<double> w(N);
    for (int n = 0; n < N; ++n) {
        double x = 2.0 * M_PI * n / (N - 1);
        w[n] = 0.35875 - 0.48829 * std::cos(x) + 0.14128 * std::cos(2.0 * x) - 0.01168 * std::cos(3.0 * x);
    }
    return w;
}

void SpectrogramChartPanel::prepareSpectrogramParams() {
    m_spectrogramData.clear();
    m_computedFrames.clear();
    m_fftWindow.clear();
    m_numFreqBins = 0;
    m_totalFrames = 0;

    if (m_samples.empty() || m_sampleRate <= 0) return;

    loadConfigParams();

    double sampleRateRatio = m_sampleRate / kStandardSampleRate;
    m_hopSize = std::max(1, static_cast<int>(std::round(m_configHopSize * sampleRateRatio)));

    double windowExp = std::log2(m_configWindowSize) + std::log2(sampleRateRatio);
    m_windowSize = static_cast<int>(std::round(std::pow(2.0, windowExp)));
    int p = 1;
    while (p < m_windowSize) p <<= 1;
    m_windowSize = p;

    m_fftWindow = makeBlackmanHarrisWindow(m_windowSize);

    int fftOutSize = m_windowSize / 2 + 1;

    m_numFreqBins = fftOutSize;

    int totalSamples = static_cast<int>(m_samples.size());
    int paddedSize = totalSamples + m_windowSize;

    m_totalFrames = (paddedSize - m_windowSize) / m_hopSize + 1;
    if (m_totalFrames <= 0) {
        m_totalFrames = 0;
        return;
    }

    m_spectrogramData.resize(m_totalFrames);
    m_computedFrames.resize(m_totalFrames, false);
}

void SpectrogramChartPanel::computeSpectrogramRange(int frameStart, int frameEnd) {
    if (m_samples.empty() || m_totalFrames <= 0 || m_fftWindow.empty()) return;

    frameStart = std::max(0, frameStart);
    frameEnd = std::min(m_totalFrames, frameEnd);
    if (frameStart >= frameEnd) return;

    int halfWindow = m_windowSize / 2;
    int fftOutSize = m_windowSize / 2 + 1;

    int totalSamples = static_cast<int>(m_samples.size());
    int paddedSize = totalSamples + m_windowSize;

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
            int srcIdx = offset + i - halfWindow;
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
                dB = m_configMinDb;
            } else {
                dB = 10.0 * std::log10(power);
            }

            double norm = (dB - m_configMinDb) / (m_configMaxDb - m_configMinDb);
            bins[b] = static_cast<float>(std::clamp(norm, 0.0, 1.0));
        }

        m_computedFrames[frame] = true;
    }

    fftw_destroy_plan(plan);
    fftw_free(fftIn);
    fftw_free(fftOut);
}

void SpectrogramChartPanel::ensureSpectrogramRange(int frameStart, int frameEnd) {
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

QRgb SpectrogramChartPanel::intensityToColor(float normalized) const {
    float v = std::clamp(normalized, 0.0f, 1.0f);
    v = std::sqrt(v);
    return m_palette.map(v);
}

void SpectrogramChartPanel::rebuildViewImage() {
    if (m_totalFrames <= 0 || m_numFreqBins <= 0 || !m_converter) {
        m_viewImage = QImage();
        return;
    }

    int w = width();
    int yaw = yAxisWidth();
    int h = height();
    int dataW = w - yaw;
    if (dataW <= 0) dataW = 1;
    if (h <= 0) h = 200;

    double totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;
    int totalFrames = m_totalFrames;

    double viewFrameStartF = (m_converter->startSec() / totalDuration) * totalFrames;
    double viewFrameEndF = (m_converter->endSec() / totalDuration) * totalFrames;
    int margin = 2;
    int frameStart = std::max(0, static_cast<int>(viewFrameStartF) - margin);
    int frameEnd = std::min(totalFrames, static_cast<int>(std::ceil(viewFrameEndF)) + margin);

    ensureSpectrogramRange(frameStart, frameEnd);

    m_viewImage = QImage(dataW, h, QImage::Format_RGB32);
    m_viewImage.fill(qRgb(0, 0, 0));

    fillImageColumns(0, dataW, dataW, h, totalFrames, totalDuration);
}

void SpectrogramChartPanel::rebuildViewImagePartial(const RegionUpdate &region) {
    if (m_totalFrames <= 0 || m_numFreqBins <= 0 || m_viewImage.isNull() || !m_converter)
        return;

    int w = width();
    int yaw = yAxisWidth();
    int dataW = w - yaw;
    int h = height();
    double totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;
    int totalFrames = m_totalFrames;

    double span = m_converter->endSec() - m_converter->startSec();
    int frameStart = static_cast<int>((m_converter->startSec() + span * region.dirtyStartCol / dataW) / totalDuration * totalFrames);
    int frameEnd = static_cast<int>(std::ceil((m_converter->startSec() + span * region.dirtyEndCol / dataW) / totalDuration * totalFrames)) + 1;
    frameStart = std::max(0, frameStart - 2);
    frameEnd = std::min(totalFrames, frameEnd + 2);
    ensureSpectrogramRange(frameStart, frameEnd);

    QRgb *bits = reinterpret_cast<QRgb *>(m_viewImage.bits());
    int stride = m_viewImage.bytesPerLine() / static_cast<int>(sizeof(QRgb));

    if (region.colShift > 0) {
        for (int y = 0; y < h; ++y)
            std::memmove(bits + y * stride, bits + y * stride + region.colShift,
                         (dataW - region.colShift) * sizeof(QRgb));
    } else {
        int shift = -region.colShift;
        for (int y = 0; y < h; ++y)
            std::memmove(bits + y * stride + shift, bits + y * stride,
                         (dataW - shift) * sizeof(QRgb));
    }

    fillImageColumns(region.dirtyStartCol, region.dirtyEndCol, dataW, h, totalFrames, totalDuration);
}

void SpectrogramChartPanel::fillImageColumns(int startX, int endX, int w, int h,
                                              int totalFrames, double totalDuration) {
    if (startX < 0 || endX > w || startX >= endX) return;

    int stride = m_viewImage.bytesPerLine() / static_cast<int>(sizeof(QRgb));

    for (int x = startX; x < endX; ++x) {
        double t = m_converter->startSec() + (m_converter->endSec() - m_converter->startSec()) * (x + 0.5) / w;
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

        QRgb *scanLine = reinterpret_cast<QRgb *>(m_viewImage.scanLine(0));
        for (int y = 0; y < h; ++y) {
            float binFrac = static_cast<float>(h - 1 - y) / (h - 1) * (m_numFreqBins - 1);
            int bin = static_cast<int>(binFrac);
            if (bin < 0 || bin >= m_numFreqBins) {
                scanLine += stride;
                continue;
            }

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

            scanLine[x] = intensityToColor(val);
            scanLine += stride;
        }
    }
}

void SpectrogramChartPanel::renderFullData(QImage &image) {
    image.fill(Qt::black);
    if (m_totalFrames <= 0 || m_numFreqBins <= 0) return;

    int imgW = image.width();
    int imgH = image.height();
    if (imgW <= 0 || imgH <= 0) return;

    // Lazily compute all FFT frames
    int totalFrames = m_totalFrames;
    ensureSpectrogramRange(0, totalFrames);

    // Render spectrogram: map each pixel column to a frame, each row to a frequency bin
    for (int x = 0; x < imgW; ++x) {
        double frameIdx = static_cast<double>(x) / imgW * totalFrames;
        int frame0 = static_cast<int>(frameIdx);
        int frame1 = std::min(frame0 + 1, totalFrames - 1);
        float frameFrac = static_cast<float>(frameIdx - frame0);

        if (frame0 < 0 || frame0 >= totalFrames) continue;
        if (!m_computedFrames[frame0]) continue;
        if (!m_computedFrames[frame1]) frame1 = frame0;

        const auto &bins0 = m_spectrogramData[frame0];
        const auto &bins1 = m_spectrogramData[frame1];
        if (bins0.empty() || bins1.empty()) continue;

        for (int y = 0; y < imgH; ++y) {
            // Map pixel row to frequency bin (y=0 = top = highest frequency)
            float binFrac = static_cast<float>(imgH - 1 - y) / (imgH - 1) * (m_numFreqBins - 1);
            int bin = static_cast<int>(binFrac);
            if (bin < 0 || bin >= m_numFreqBins) continue;

            // Bilinear interpolation in frequency dimension
            float val0, val1;
            if (bin + 1 < m_numFreqBins) {
                float frac = binFrac - bin;
                val0 = bins0[bin] * (1.0f - frac) + bins0[bin + 1] * frac;
                val1 = bins1[bin] * (1.0f - frac) + bins1[bin + 1] * frac;
            } else {
                val0 = bins0[bin];
                val1 = bins1[bin];
            }

            // Linear interpolation in time dimension
            float val = val0 * (1.0f - frameFrac) + val1 * frameFrac;

            image.setPixelColor(x, y, QColor(intensityToColor(val)));
        }
    }
}

double SpectrogramChartPanel::dataDurationSec() const {
    if (m_samples.empty() || m_sampleRate <= 0) return 0.0;
    // m_samples is stereo interleaved
    return static_cast<double>(m_samples.size()) / (m_sampleRate * 2.0);
}

} // namespace dstools
