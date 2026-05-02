#pragma once

#include <QPainter>
#include <QRect>
#include <QPoint>
#include <QColor>
#include <QSet>

#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>

#include <cstdint>
#include <memory>
#include <set>
#include <map>
#include <vector>

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {

/// Color constants for the piano roll.
namespace Colors {
    static const QColor Background("#1A1A22");
    static const QColor GridSemitone("#26262E");
    static const QColor GridCents("#1F1F28");
    static const QColor BarLine("#3A3A44");
    static const QColor RulerBg("#22222C");
    static const QColor RulerTick("#555555");
    static const QColor RulerText("#9898A8");
    static const QColor PianoWhite("#E8E8E8");
    static const QColor PianoBlack("#2A2A2A");
    static const QColor PianoBg("#22222C");

    static const QColor NoteDefault("#4A8FD9");
    static const QColor NoteDefaultTop("#5A9FE9");
    static const QColor NoteDefaultBottom("#4485C9");
    static const QColor NoteBorder("#5EA2ED");
    static const QColor NoteSlur("#2E6BB0");
    static const QColor NoteSlurBorder("#4A8FCC");
    static const QColor NoteRestFill(0x3A, 0x3A, 0x44, int(0.35 * 255));
    static const QColor NoteRestBorder(0x5A, 0x5A, 0x64, int(0.5 * 255));
    static const QColor NoteText("#FFFFFF");

    static const QColor F0Default("#FF8C42");
    static const QColor F0Dimmed(0xFF, 0x8C, 0x42, int(0.4 * 255));
    static const QColor F0Selected("#FFB366");

    static const QColor NoteSelectedTop("#FFD066");
    static const QColor NoteSelectedBottom("#E8B040");
    static const QColor NoteSelectedBorder("#FFE088");
    static const QColor NoteSelectedGlow(0xFF, 0xD0, 0x66, 60);
    static const QColor SnapGuide("#FFD066");
    static const QColor RubberBandBorder(0x4A, 0x8F, 0xD9, 180);
    static const QColor RubberBandFill(0x4A, 0x8F, 0xD9, 38);

    static const QColor Playhead("#EF5350");
    static const QColor PlayheadIdle("#F0F1F2");
}

/// Read-only snapshot of view state needed by the renderer.
struct RenderState {
    // Data
    const DSFile *dsFile = nullptr;

    // Viewport
    double hScale = 100.0;
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
    const std::vector<int32_t> *originalF0 = nullptr;

    // Layout constants
    static constexpr int PianoWidth = 52;
    static constexpr int RulerHeight = 24;
    static constexpr int MinMidi = 24;
    static constexpr int MaxMidi = 96;

    // Coordinate conversion helpers (inlined for performance)
    double timeToX(double time) const { return time * hScale + PianoWidth; }
    double xToTime(double x) const { return (x - PianoWidth) / hScale; }
    double midiToY(double midi) const { return (MaxMidi - midi) * vScale + RulerHeight; }
    double yToMidi(double y) const { return MaxMidi - (y - RulerHeight) / vScale; }
    int sceneXToWidget(double sceneX) const { return static_cast<int>(sceneX - scrollX); }
    int sceneYToWidget(double sceneY) const { return static_cast<int>(sceneY - scrollY); }
    double widgetXToScene(int wx) const { return wx + scrollX; }
    double widgetYToScene(int wy) const { return wy + scrollY; }
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
