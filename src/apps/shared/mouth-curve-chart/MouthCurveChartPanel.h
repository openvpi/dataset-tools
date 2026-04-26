#pragma once

#include "dstools/MouthCurve.h"

#include <ChartPanelBase.h>
#include <ChartPanelTypes.h>

namespace dstools {

    class MouthCurveChartPanel : public chart::ChartPanelBase {
        Q_OBJECT
    public:
        explicit MouthCurveChartPanel(dsfw::widgets::ViewportController *viewport, QWidget *parent = nullptr);

        static void registerChartConfig();

        void setData(const MouthCurve &curve);

    protected:
        void drawContent(QPainter &painter, const chart::ChartCoordinate &coord) override;
        void rebuildCache(const chart::RegionUpdate &region) override;
        bool supportsVerticalZoom() const override {
            return true;
        }
        double amplitudeMinForZoom() const override;
        double amplitudeMaxForZoom() const override;
        void paintYAxisContent(QPainter &painter, const QRect &chartRect) override;

    private:
        void loadConfigParams();
        void paintCurve(QPainter &painter);

        MouthCurve m_curve;

        static constexpr int kYTickCount = 6;
        static constexpr float kCurveMin = 0.0f;
        static constexpr float kCurveMax = 1.0f;
        static constexpr double kDefaultAmplitudeMin = 0.5;
        static constexpr double kDefaultAmplitudeMax = 10.0;

        double m_configAmplitudeMin = kDefaultAmplitudeMin;
        double m_configAmplitudeMax = kDefaultAmplitudeMax;
    };

} // namespace dstools