#pragma once

#include "ChartCoordinate.h"
#include "ChartPanelTypes.h"

#include <QImage>
#include <QPoint>
#include <QWidget>
#include <cstring>
#include <dstools/Constants.h>
#include <dsfw/widgets/PlayWidget.h>
#include <dstools/TimePos.h>
#include <utility>
#include <vector>

class QUndoStack;

namespace dstools {
    namespace chart {

        using dsfw::widgets::ViewportController;

        class IBoundaryModel;
        class BoundaryDragController;

        class ChartPanelBase : public QWidget, public IChartPanel {
            Q_OBJECT

        public:
            explicit ChartPanelBase(const QString &id, ViewportController *viewport, QWidget *parent = nullptr);

            QString chartId() const override {
                return m_id;
            }
            QWidget *widget() override {
                return this;
            }

            void onViewportUpdate(const ChartCoordinate &conv, int pixelWidth) override;
            QWidget *createYAxisLabelWidget(QWidget *parent) override;
            void onBoundaryModelInvalidated() override {
                update();
            }
            void onPlayheadUpdate(const PlayheadState &state) override;

            std::pair<double, double> findSurrounding(double timeSec) const override;

            void onActiveTierChanged(int tierIndex) override;
            std::pair<int, int> hitTestBoundary(int x) const;

            void setBoundaryModel(IBoundaryModel *model) {
                m_boundaryModel = model;
            }
            void setDragController(BoundaryDragController *ctrl) {
                m_dragController = ctrl;
            }
            void setPlayWidget(dsfw::widgets::PlayWidget *pw) {
                m_playWidget = pw;
            }
            void setUndoStack(QUndoStack *stack) {
                m_undoStack = stack;
            }

            void setAudioData(const std::vector<float> &samples, int sampleRate);

            virtual void setPlayhead(double sec) {
                Q_UNUSED(sec)
            }

        signals:
            void visibleStateChanged(bool visible);
            void hoveredBoundaryChanged(int boundaryIndex);
            void boundaryDragging();
            void entryScrollRequested(int delta);
            void positionClicked(double sec);
            void boundaryDragFinished(int tierIndex, int boundaryIndex, dstools::TimePos time);
            void verticalContentScrolled();

        protected:
            // ========== 新增纯虚方法（子类必须实现） ==========
            /// @brief Render the full data into a pre-allocated image.
            ///        The image is sized according to fullDataImageWidth() x fullDataImageHeight().
            virtual void renderFullData(QImage &image) = 0;
            /// @brief Return the total data duration in seconds.
            virtual double dataDurationSec() const = 0;

            // ========== 新增可选虚方法（子类可覆盖） ==========
            /// @brief Width of the full-data image. Default: widget width().
            virtual int fullDataImageWidth() const { return width(); }
            /// @brief Height of the full-data image. Default: widget height().
            virtual int fullDataImageHeight() const { return height(); }

            // ========== 现有虚方法 ==========
            virtual void rebuildCache(const RegionUpdate &region) {
                Q_UNUSED(region)
            }
            virtual void drawContent(QPainter &painter, const ChartCoordinate &coord) = 0;
            virtual void onVerticalZoom(double factor);
            virtual void onAudioDataChanged();
            virtual bool supportsVerticalZoom() const {
                return false;
            }
            virtual double amplitudeMinForZoom() const {
                return 0.1;
            }
            virtual double amplitudeMaxForZoom() const {
                return 20.0;
            }

            [[nodiscard]] virtual int yAxisWidth() const {
                return 0;
            }
            virtual void paintYAxisContent(QPainter &painter, const QRect &chartRect) {
                Q_UNUSED(painter)
                Q_UNUSED(chartRect)
            }

            static constexpr int kYAxisPadding = 4;

            void paintYAxisTicks(QPainter &painter, const QRect &chartRect, double minValue, double maxValue,
                                 int tickCount, int decimals, const QString &suffix = {}) const;
            void paintOutOfBoundsOverlay(QPainter &painter, const QRect &clipRect, const ChartCoordinate &coord) const;
            [[nodiscard]] QRect chartContentRect() const {
                int yaw = yAxisWidth();
                return QRect(yaw, 0, width() - yaw, height());
            }
            [[nodiscard]] double clientXToTime(int clientX) const {
                return m_converter ? m_converter->xToTime(clientX, m_dataPixelWidth) : 0.0;
            }

            [[nodiscard]] double timeToClientX(double timeSec) const {
                return m_converter ? m_converter->timeToX(timeSec, m_dataPixelWidth) : 0.0;
            }

            void drawEmptyState(QPainter &painter, const QString &msg);

            template <typename T>
            static void shiftCache(std::vector<T> &cache, int width, int colShift) {
                if (colShift > 0)
                    std::memmove(&cache[0], &cache[colShift], (width - colShift) * sizeof(T));
                else if (colShift < 0)
                    std::memmove(&cache[-colShift], &cache[0], (width + colShift) * sizeof(T));
            }

            void paintEvent(QPaintEvent *event) override;
            void resizeEvent(QResizeEvent *event) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void contextMenuEvent(QContextMenuEvent *event) override;
            void wheelEvent(QWheelEvent *event) override;

            std::vector<float> m_samples;
            int m_sampleRate = constants::kDefaultSampleRate;
            const ChartCoordinate *m_converter = nullptr;
            int m_dataPixelWidth = 0;
            bool m_cacheDirty = true;
            RegionUpdate m_pendingRegion;
            double m_amplitudeScale = 1.0;

            // ========== 新增成员 ==========
            QImage m_fullDataImage;
            bool m_fullDataDirty = true;

        private:
            void playSegmentBetween(double startSec, double endSec);

            QString m_id;
            ViewportController *m_viewport = nullptr;
            IBoundaryModel *m_boundaryModel = nullptr;
            BoundaryDragController *m_dragController = nullptr;
            dsfw::widgets::PlayWidget *m_playWidget = nullptr;
            QUndoStack *m_undoStack = nullptr;

            std::pair<int, int> m_hoveredBoundary = {-1, -1};

            static constexpr int kBoundaryHitWidth = 12;
        };

    } // namespace chart
} // namespace dstools
