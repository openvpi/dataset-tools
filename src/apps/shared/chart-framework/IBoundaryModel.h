#pragma once

#include <QString>
#include <dsfw/TimePos.h>

namespace dstools {

        struct OutOfBoundsRange {
            double startSec;
            double endSec;
        };

        class IBoundaryModel {
    public:
        static constexpr int kInterfaceVersion = 1;

        virtual ~IBoundaryModel() = default;

            [[nodiscard]] virtual int tierCount() const = 0;

            [[nodiscard]] virtual QString tierName(int /*tierIndex*/) const {
                return {};
            }

            [[nodiscard]] virtual int activeTierIndex() const = 0;

            [[nodiscard]] virtual int boundaryCount(int tierIndex) const = 0;

            [[nodiscard]] virtual dsfw::TimePos boundaryTime(int tierIndex, int boundaryIndex) const = 0;

            virtual void moveBoundary(int tierIndex, int boundaryIndex, dsfw::TimePos newTime) = 0;

            [[nodiscard]] virtual dsfw::TimePos totalDuration() const = 0;

            [[nodiscard]] virtual bool supportsBinding() const {
                return false;
            }

            [[nodiscard]] virtual bool supportsInsert() const {
                return false;
            }

            [[nodiscard]] virtual std::vector<OutOfBoundsRange> getOutOfBoundsRanges(int /*tierIndex*/) const {
                return {};
            }

            [[nodiscard]] virtual dsfw::TimePos clampBoundaryTime(int /*tierIndex*/, int /*boundaryIndex*/,
                                                            dsfw::TimePos proposedTime) const {
                return proposedTime;
            }

            [[nodiscard]] virtual dsfw::TimePos snapToNearestBoundary(int /*tierIndex*/, dsfw::TimePos proposedTime,
                                                                dsfw::TimePos /*snapThreshold*/) const {
                return proposedTime;
            }

            [[nodiscard]] virtual QString intervalText(int /*tierIndex*/, int /*intervalIndex*/) const {
                return {};
            }

            [[nodiscard]] virtual dsfw::TimePos snapToNearestBoundaryPixels(int tierIndex, dsfw::TimePos proposedTime,
                                                                      double pixelsPerSecond,
                                                                      double pixelThreshold) const {
                if (pixelsPerSecond <= 0)
                    return proposedTime;
                dsfw::TimePos timeThreshold = static_cast<dsfw::TimePos>(pixelThreshold / pixelsPerSecond * 1000000.0);
                return snapToNearestBoundary(tierIndex, proposedTime, timeThreshold);
            }
        };

    } // namespace dstools