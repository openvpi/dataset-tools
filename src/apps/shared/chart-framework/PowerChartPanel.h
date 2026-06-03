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

            void paintYAxisContent(QPainter &painter, const QRect &chartRect) override;
            void onAudioDataChanged() override;

            // F-01: 新增纯虚方法实现
            void renderFullData(QImage &image) override;
            double dataDurationSec() const override;

        private:
            void loadConfigParams();

            static constexpr float kDefaultMinDb = -96.0f;
            static constexpr float kDefaultMaxDb = 0.0f;
            static constexpr int kDefaultWindowSize = 2048;

            float m_configMinDb = kDefaultMinDb;
            float m_configMaxDb = kDefaultMaxDb;
            int m_configWindowSize = kDefaultWindowSize;
        };

    } // namespace chart
} // namespace dstools