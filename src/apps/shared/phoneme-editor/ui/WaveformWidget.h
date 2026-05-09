#pragma once

#include "AudioChartWidget.h"

#include <vector>

class QPainter;
class QPixmap;

namespace dstools {
namespace phonemelabeler {

class WaveformWidget : public AudioChartWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~WaveformWidget() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);
    void loadAudio(const QString &path);
    void setPlayhead(double sec);
    void setBoundaryOverlayEnabled(bool enabled);
    void rebuildMinMaxCache();

    void rebuildCache() override;

signals:
    void positionClicked(double sec);

public slots:
    void onPlayheadChanged(double sec);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void onViewportDragStart(double timeSec) override;
    void onVerticalZoom(double factor) override;

private:
    void drawWaveform(QPainter &painter);
    void drawDbAxis(QPainter &painter);
    void drawPlayCursor(QPainter &painter);

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    struct MinMaxPair {
        float min;
        float max;
    };
    std::vector<MinMaxPair> m_minMaxCache;
    double m_cacheResolution = 0;

    double m_playhead = -1.0;
    double m_amplitudeScale = 1.0;
    bool m_boundaryOverlayEnabled = true;
};

} // namespace phonemelabeler
} // namespace dstools
