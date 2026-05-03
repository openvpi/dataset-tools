/// @file MelSpectrogramWidget.h
/// @brief Mel-scale spectrogram widget for WaveformPanel (foldable).

#pragma once

#include <QImage>
#include <QWidget>
#include <vector>

#include <dstools/ViewportController.h>

namespace dstools {
namespace waveform {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

/// @brief Displays a Mel-scale spectrogram below the waveform.
///
/// Designed for the WaveformPanel. Uses FFTW3 to compute mel filterbank
/// energies. Can be collapsed/expanded via setVisible().
class MelSpectrogramWidget : public QWidget {
    Q_OBJECT

public:
    explicit MelSpectrogramWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~MelSpectrogramWidget() override;

    /// Set audio sample data for spectrogram computation.
    void setAudioData(const std::vector<float> &samples, int sampleRate);

    /// Update the viewport state.
    void setViewport(const ViewportState &state);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void computeMelSpectrogram();
    void rebuildViewImage();

    [[nodiscard]] static std::vector<double> makeHannWindow(int N);
    [[nodiscard]] static std::vector<std::vector<double>> melFilterbank(int numFilters,
                                                                        int fftSize,
                                                                        int sampleRate,
                                                                        double fMin,
                                                                        double fMax);

    ViewportController *m_viewport = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    // Mel spectrogram data: [frame][melBin], normalized 0..1
    std::vector<std::vector<float>> m_melData;
    int m_hopSize = 512;
    int m_windowSize = 2048;
    int m_numMelBins = 80;

    // Cached view
    QImage m_viewImage;
    double m_cachedViewStart = -1.0;
    double m_cachedViewEnd = -1.0;
    int m_cachedWidth = 0;
    int m_cachedHeight = 0;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    static constexpr double kMinDb = -80.0;
    static constexpr double kMaxDb = 0.0;
    static constexpr double kMaxFreq = 8000.0;
};

} // namespace waveform
} // namespace dstools
