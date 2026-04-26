#pragma once

#include "ChartPanelBase.h"

#include <QWidget>

namespace dstools {
    namespace chart {

        class WaveformChartPanel : public ChartPanelBase {
            Q_OBJECT

        public:
            explicit WaveformChartPanel(ViewportController *viewport, QWidget *parent = nullptr);

            static void registerChartConfig();

            void setColor(const QColor &color);

            void drawContent(QPainter &painter, const ChartCoordinate &coord) override;
        void onAudioDataChanged() override;
        void paintYAxisContent(QPainter &painter, const QRect &chartRect) override;

            void rebuildCache(const RegionUpdate &region) override;

        private:
            void loadConfigParams();
            void rebuildWaveformCache();
            void rebuildWaveformCachePartial(const RegionUpdate &region);
            void drawWaveform(QPainter &painter);

            std::vector<float> m_selectionMask;

            QColor m_waveColor{100, 180, 255};
            QColor m_loudColor{51, 153, 255};
            QColor m_quietColor{120, 160, 200};

            double m_opacity = 0.16;
            float m_loudnessRef = 0.5f;

            std::vector<float> m_yMax;
            std::vector<float> m_yMin;
            std::vector<float> m_rms;
            std::vector<float> m_loudnessMask;
        };

    } // namespace chart
} // namespace dstools