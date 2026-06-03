#pragma once

#include <ChartCoordinate.h>
#include <QColor>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QSet>
#include <cstdint>
#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>
#include <map>
#include <memory>
#include <set>
#include <vector>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            /// Read-only snapshot of view state needed by the renderer.
            struct RenderState {
                // Data
                const DsPitchDocument *dsFile = nullptr;

                // Viewport
                double vScale = 20.0;
                double scrollX = 0.0;
                double scrollY = 0.0;
                double audioDuration = 0.0;

                // Playhead
                double playheadPos = 0.0;
                bool isPlaying = false;

                // Selection
                const std::set<int> *selectedNotes = nullptr;

                // Display options
                bool showPitchTextOverlay = false;
                bool showPhonemeTexts = true;
                bool showCrosshairAndPitch = true;

                // Mouse
                QPoint mousePos;

                // Note drag preview
                bool draggingNote = false;
                int dragAccumulatedCents = 0;
                const std::map<int, double> *dragOrigMidi = nullptr;

                // Rubber band
                bool rubberBandActive = false;
                QRect rubberBandRect;

                // A/B comparison
                bool abComparisonActive = false;
                const std::vector<float> *originalF0 = nullptr;

                chart::ChartCoordinate coord; // Unified coordinate interface

                // Layout constants (configurable via ChartConfigRegistry)
                static constexpr int RulerHeight = 0;

                int pianoWidth = 52;
                int scrollBarSize = 14;
                int minMidi = 24;
                int maxMidi = 96;
                double modulationDragSensitivity = 80.0;

                int contentLeft = 52; // initialized from pianoWidth in buildRenderState

                // Coordinate conversion helpers (inlined for performance)
                double timeToX(double time) const {
                    return coord.timeToX(time);
                }
                double xToTime(double x) const {
                    return coord.xToTime(static_cast<int>(std::round(x)));
                }
                double midiToY(double midi) const {
                    return coord.midiToY(midi);
                }
                double yToMidi(double y) const {
                    return coord.yToMidi(y);
                }
                int sceneXToWidget(double sceneX) const {
                    return static_cast<int>(sceneX);
                }
                int sceneYToWidget(double sceneY) const {
                    return coord.sceneYToWidget(sceneY);
                }
                double widgetXToScene(int wx) const {
                    return static_cast<double>(wx);
                }
                double widgetYToScene(int wy) const {
                    return coord.widgetYToScene(wy);
                }

                double pixelsPerSecond() const {
                    return (coord.pixelWidth > 0 && coord.viewEnd > coord.viewStart)
                               ? coord.pixelWidth / (coord.viewEnd - coord.viewStart)
                               : 100.0;
                }
            };

            /// Stateless renderer — all draw methods take a QPainter and const RenderState.
            class PianoRollRenderer {
            public:
                static void drawGrid(QPainter &p, int w, int h, const RenderState &s);
                static void drawPianoKeys(QPainter &p, int h, const RenderState &s);
                static void drawRuler(QPainter &p, int w, const RenderState &s);
                static void drawNotes(QPainter &p, int w, int h, const RenderState &s);
                static void drawF0Curve(QPainter &p, int w, int h, const RenderState &s);
                static void drawPlayhead(QPainter &p, int w, int h, const RenderState &s);
                static void drawCrosshair(QPainter &p, int w, int h, const RenderState &s);
                static void drawRubberBand(QPainter &p, const RenderState &s);
                static void drawSnapGuide(QPainter &p, int w, int h, const RenderState &s);
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools
