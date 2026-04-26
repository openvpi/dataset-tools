#pragma once

#include "ChartCoordinate.h"

#include <QObject>
#include <dstools/TimePos.h>
#include <vector>

class QUndoStack;

namespace dstools {
    namespace chart {

        class IBoundaryModel;

        struct AlignedBoundary {
            int tierIndex;
            int boundaryIndex;
            TimePos time;
        };

        class BoundaryDragController : public QObject {
            Q_OBJECT

        public:
            explicit BoundaryDragController(QObject *parent = nullptr);

            void setBindingEnabled(bool enabled);
            void setSnapEnabled(bool enabled);
            void setToleranceUs(TimePos toleranceUs);
            void setSnapThresholdUs(TimePos thresholdUs);
            void setCoordConverter(const ChartCoordinate *conv, int pixelWidth);
            void setPixelSnapThreshold(double pixelThreshold);

            [[nodiscard]] bool isBindingEnabled() const {
                return m_bindingEnabled;
            }
            [[nodiscard]] bool isSnapEnabled() const {
                return m_snapEnabled;
            }
            [[nodiscard]] TimePos snapThresholdUs() const {
                return m_snapThresholdUs;
            }
            [[nodiscard]] double pixelSnapThreshold() const {
                return m_pixelSnapThreshold;
            }

            [[nodiscard]] bool isDragging() const {
                return m_dragging;
            }
            [[nodiscard]] int draggedTier() const {
                return m_draggedTier;
            }
            [[nodiscard]] int draggedBoundary() const {
                return m_draggedBoundary;
            }

            void startDrag(int tierIndex, int boundaryIndex, IBoundaryModel *model);

            void updateDrag(TimePos proposedTime);

            void endDrag(TimePos finalTime, QUndoStack *undoStack);

            void cancelDrag();

            [[nodiscard]] size_t partnerCount() const {
                return m_partners.size();
            }

        signals:
            void dragStarted(int tierIndex, int boundaryIndex);
            void dragging(int tierIndex, int boundaryIndex, TimePos currentTime);
            void dragFinished(int tierIndex, int boundaryIndex, TimePos finalTime);
            void bindingEnabledChanged(bool enabled);
            void snapEnabledChanged(bool enabled);

        private:
            std::vector<AlignedBoundary> findPartners(int sourceTier, int sourceBoundaryIndex) const;

            void restoreOriginals();

            [[nodiscard]] double PPS() const {
                return m_converter ? m_converter->pixelsPerSecond(m_pixelWidth) : 100.0;
            }

            bool m_bindingEnabled = true;
            bool m_snapEnabled = true;
            TimePos m_toleranceUs = 5000;
            TimePos m_snapThresholdUs = 5000;
            const ChartCoordinate *m_converter = nullptr;
            int m_pixelWidth = 0;
            double m_pixelSnapThreshold = 10.0;

            bool m_dragging = false;
            int m_draggedTier = -1;
            int m_draggedBoundary = -1;
            TimePos m_dragStartTime = 0;
            IBoundaryModel *m_model = nullptr;

            std::vector<AlignedBoundary> m_partners;
            std::vector<TimePos> m_partnerStartTimes;
        };

    } // namespace chart
} // namespace dstools