#pragma once

#include "ChartPanelBase.h"

#include <QWidget>

namespace dstools {
    namespace chart {

        class PowerChartPanel : public ChartPanelBase {
            Q_OBJECT

        public:
            explicit PowerChartPanel(ViewportController *viewport, QWidget *parent = nullptr);

            static void registerChartConfig();

            void drawContent(QPainter &painter, const ChartCoordinate &coord) override;
            void paintYAxisContent(QPainter &painter, const QRect &chartRect) override;
            void onAudioDataChanged() override;
            void rebuildCache(const RegionUpdate &region) override;

            // F-01: 新增纯虚方法实现
            void renderFullData(QImage &image) override;
            double dataDurationSec() const override;

        private:
            void loadConfigParams();
            void rebuildPowerCache();
            void rebuildPowerCachePartial(const RegionUpdate &region);
            void drawPower(QPainter &painter);

            static constexpr float kDefaultMinDb = -96.0f;
            static constexpr float kDefaultMaxDb = 0.0f;
            static constexpr int kDefaultWindowSize = 2048;

            float m_configMinDb = kDefaultMinDb;
            float m_configMaxDb = kDefaultMaxDb;
            int m_configWindowSize = kDefaultWindowSize;

            std::vector<float> m_powerCache;
        };

    } // namespace chart
} // namespace dstools
