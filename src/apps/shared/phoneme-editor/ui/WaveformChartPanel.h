#pragma once

#include "ChartPanelBase.h"

#include <vector>

namespace dstools {
    namespace phonemelabeler {

        class WaveformChartPanel : public ChartPanelBase {
            Q_OBJECT

        public:
            explicit WaveformChartPanel(ViewportController *viewport, QWidget *parent = nullptr);

            void setAudioData(const std::vector<float> &samples, int sampleRate);
            void setPlayhead(double sec) override;

            QString chartId() const override {
                return QStringLiteral("waveform");
            }

        protected:
            void rebuildCache() override;
            void drawContent(QPainter &painter) override;
            void onVerticalZoom(double factor) override;
            void resizeEvent(QResizeEvent *event) override;

        private:
            void drawDbAxis(QPainter &painter);
            void rebuildMinMaxCache();

            std::vector<float> m_samples;
            int m_sampleRate = 44100;

            struct MinMaxPair {
                float min;
                float max;
            };
            std::vector<MinMaxPair> m_minMaxCache;
            double m_cacheViewDuration = 0.0;
            double m_amplitudeScale = 1.0;
        };

    } // namespace phonemelabeler
} // namespace dstools