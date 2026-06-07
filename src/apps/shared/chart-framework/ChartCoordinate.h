#pragma once

#include <cmath>
#include <dsfw/widgets/ViewportController.h>

namespace dstools {

        using dsfw::widgets::ViewportState;

        struct ChartCoordinate {
            double viewStart = 0.0;
            double viewEnd = 1.0;
            int pixelWidth = 0;

            int timeToX(double timeSec) const {
                return static_cast<int>(timeToX(timeSec, pixelWidth));
            }

            double xToTime(int x) const {
                if (pixelWidth <= 0)
                    return viewStart;
                if (x >= pixelWidth)
                    return viewEnd;
                return xToTime(x, pixelWidth);
            }

            double timeToX(double timeSec, int width) const {
                if (width <= 0)
                    return 0.0;
                double dur = viewEnd - viewStart;
                if (dur <= 0.0)
                    return 0.0;
                return std::round((timeSec - viewStart) / dur * width);
            }

            double xToTime(int x, int width) const {
                if (width <= 0)
                    return viewStart;
                return viewStart + x * (viewEnd - viewStart) / width;
            }

            double pixelsPerSecond(int width) const {
                double dur = viewEnd - viewStart;
                if (width <= 0 || dur <= 0.0)
                    return 0.0;
                return static_cast<double>(width) / dur;
            }

            double startSec() const {
                return viewStart;
            }
            double endSec() const {
                return viewEnd;
            }
            double duration() const {
                return viewEnd - viewStart;
            }

            void update(const ViewportState &state) {
                viewStart = state.startSec;
                viewEnd = state.endSec;
            }

            int contentTop = 0;
            double vScale = 20.0;
            static constexpr int MinMidi = 24;
            static constexpr int MaxMidi = 96;

            double midiToY(double midi) const {
                return (MaxMidi - midi) * vScale + contentTop;
            }

            double yToMidi(double y) const {
                return MaxMidi - (y - contentTop) / vScale;
            }

            double scrollX = 0.0;
            double scrollY = 0.0;

            int sceneXToWidget(double sx) const {
                return static_cast<int>(sx - scrollX);
            }
            double widgetXToScene(int wx) const {
                return wx + scrollX;
            }
            int sceneYToWidget(double sy) const {
                return static_cast<int>(sy - scrollY);
            }
            double widgetYToScene(int wy) const {
                return wy + scrollY;
            }
        };

    } // namespace dstools