#pragma once

#include "ChartPanelBase.h"
#include "SpectrogramColorPalette.h"

#include <QImage>
#include <QWidget>
#include <fftw3.h>

namespace dstools {
    namespace dstools {

        class SpectrogramChartPanel : public ChartPanelBase {
            Q_OBJECT

        public:
            explicit SpectrogramChartPanel(ViewportController *viewport, QWidget *parent = nullptr);

            static void registerChartConfig();

            void setColorPalette(const SpectrogramColorPalette &palette);
            void rebuildCache(const RegionUpdate &region) override;
            void drawContent(QPainter &painter, const ChartCoordinate &coord) override;
            void paintYAxisContent(QPainter &painter, const QRect &chartRect) override;
            void onAudioDataChanged() override;

            // F-01: 新增纯虚方法实现
            void renderFullData(QImage &image) override;
            double dataDurationSec() const override;

            // F-04: 完整频谱图尺寸 — 受 QImage 最大宽度限制
            int fullDataImageWidth() const override {
                return std::min(m_totalFrames, 32767);
            }
            int fullDataImageHeight() const override {
                return m_numFreqBins;
            }

        private:
            void loadConfigParams();
            void prepareSpectrogramParams();
            void computeSpectrogramRange(int frameStart, int frameEnd);
            void ensureSpectrogramRange(int frameStart, int frameEnd);
            void rebuildViewImage();
            void rebuildViewImagePartial(const RegionUpdate &region);
            void fillImageColumns(int startX, int endX, int w, int h, int totalFrames, double totalDuration);
            QRgb intensityToColor(float normalized) const;
            static std::vector<double> makeBlackmanHarrisWindow(int N);

            static constexpr double kStandardSampleRate = 44100.0;

            int m_configHopSize = 256;
            int m_configWindowSize = 2048;
            double m_configMinDb = -80.0;
            double m_configMaxDb = 0.0;

            int m_hopSize = 256;
            int m_windowSize = 2048;
            std::vector<double> m_fftWindow;
            int m_numFreqBins = 0;
            int m_totalFrames = 0;

            std::vector<std::vector<float>> m_spectrogramData;
            std::vector<bool> m_computedFrames;

            QImage m_viewImage;
            RegionUpdate m_pendingRegion{};
            bool m_cacheDirty = false;

            SpectrogramColorPalette m_palette{SpectrogramColorPalette::orangeYellow()};
        };

    } // namespace dstools
} // namespace dstools
