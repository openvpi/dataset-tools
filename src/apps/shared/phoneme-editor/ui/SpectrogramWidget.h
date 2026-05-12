#pragma once

#include "AudioChartWidget.h"
#include "SpectrogramColorPalette.h"

#include <QImage>
#include <vector>

class QPainter;

namespace dstools {
namespace phonemelabeler {

class SpectrogramWidget : public AudioChartWidget {
    Q_OBJECT

public:
    explicit SpectrogramWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~SpectrogramWidget() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);
    void setColorPalette(const SpectrogramColorPalette &palette);
    [[nodiscard]] const SpectrogramColorPalette &colorPalette() const { return m_palette; }

    void rebuildCache() override;

signals:
    void visibleStateChanged(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void prepareSpectrogramParams();
    void computeSpectrogramRange(int frameStart, int frameEnd);
    void ensureSpectrogramRange(int frameStart, int frameEnd);
    void rebuildViewImage();
    static std::vector<double> makeBlackmanHarrisWindow(int N);
    [[nodiscard]] QRgb intensityToColor(float normalized) const;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;
    SpectrogramColorPalette m_palette = SpectrogramColorPalette::orangeYellow();

    std::vector<std::vector<float>> m_spectrogramData;
    std::vector<bool> m_computedFrames;
    std::vector<double> m_fftWindow;
    int m_totalFrames = 0;
    int m_hopSize = 110;
    int m_windowSize = 512;
    int m_numFreqBins = 0;

    QImage m_viewImage;
    double m_cachedViewStart = -1.0;
    double m_cachedViewEnd = -1.0;
    int m_cachedWidth = 0;
    int m_cachedHeight = 0;

    static constexpr int kStandardHopSize = 256;
    static constexpr int kStandardWindowSize = 2048;
    static constexpr double kStandardSampleRate = 44100.0;
    static constexpr double kMaxFrequency = 8000.0;
    static constexpr double kMinIntensityDb = -80.0;
    static constexpr double kMaxIntensityDb = 0.0;
};

} // namespace phonemelabeler
} // namespace dstools