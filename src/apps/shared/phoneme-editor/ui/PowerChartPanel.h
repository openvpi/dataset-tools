#pragma once

#include "ChartPanelBase.h"

#include <vector>

namespace dstools {
namespace phonemelabeler {

class PowerChartPanel : public ChartPanelBase {
    Q_OBJECT

public:
    explicit PowerChartPanel(ViewportController *viewport, QWidget *parent = nullptr);

    void setAudioData(const std::vector<float> &samples, int sampleRate) override;

    QString chartId() const override { return QStringLiteral("power"); }

protected:
    void rebuildCache() override;
    void drawContent(QPainter &painter) override;

private:
    void rebuildPowerCache();
    void drawPower(QPainter &painter);
    void drawReferenceLines(QPainter &painter);

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    std::vector<float> m_powerCache;

    static constexpr int kUnitSize = 60;
    static constexpr int kWindowSize = 300;
    static constexpr float kMinPower = -48.0f;
    static constexpr float kMaxPower = 0.0f;
    static constexpr float kRefValue = 32768.0f;
};

} // namespace phonemelabeler
} // namespace dstools