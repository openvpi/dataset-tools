#include "PianoRollRenderer.h"
#include "PitchProcessor.h"
#include "DSFile.h"

#include <QPainterPath>

#include <dstools/CurveTools.h>
#include <dstools/PitchUtils.h>

#include <cmath>
#include <algorithm>

namespace dstools {
namespace pitchlabeler {
namespace ui {

using dstools::parseNoteName;
using dstools::midiToNoteName;
using dstools::midiToNoteString;
using dstools::midiToFreq;
using dstools::freqToMidi;

// ============================================================================
// Timescale level table (mirrors TimeRulerWidget's kLevels + findLevel)
// ============================================================================

struct RulerLevel {
    double majorSec;
    double minorSec;
};

static const RulerLevel kRulerLevels[] = {
    {0.001,  0.0005},   // 1ms / 0.5ms
    {0.005,  0.001},    // 5ms / 1ms
    {0.01,   0.005},    // 10ms / 5ms
    {0.05,   0.01},     // 50ms / 10ms
    {0.1,    0.05},     // 100ms / 50ms
    {0.2,    0.1},      // 200ms / 100ms
    {0.5,    0.1},      // 500ms / 100ms
    {1.0,    0.5},      // 1s / 500ms
    {2.0,    0.5},      // 2s / 500ms
    {5.0,    1.0},      // 5s / 1s
    {10.0,   2.0},      // 10s / 2s
    {15.0,   5.0},      // 15s / 5s
    {30.0,   10.0},     // 30s / 10s
    {60.0,   15.0},     // 1min / 15s
    {300.0,  60.0},     // 5min / 1min
    {900.0,  300.0},    // 15min / 5min
    {3600.0, 900.0},    // 1h / 15min
};
static constexpr int kRulerLevelCount = static_cast<int>(std::size(kRulerLevels));
static constexpr double kRulerMinMinorStepPx = 25.0;

static const RulerLevel &findRulerLevel(double pps) {
    for (int i = 0; i < kRulerLevelCount; ++i) {
        if (kRulerLevels[i].minorSec * pps >= kRulerMinMinorStepPx)
            return kRulerLevels[i];
    }
    return kRulerLevels[kRulerLevelCount - 1];
}

// ============================================================================
// Grid
// ============================================================================

void PianoRollRenderer::drawGrid(QPainter &p, int w, int h, const RenderState &s) {
    p.save();
    p.setClipRect(RenderState::PianoWidth, RenderState::RulerHeight,
                  w - RenderState::PianoWidth, h - RenderState::RulerHeight);

    QPen penSemitone(Colors::GridSemitone, 1);
    for (int midi = RenderState::MinMidi; midi <= RenderState::MaxMidi; ++midi) {
        double sceneY = s.midiToY(midi + 0.5);
        int wy = s.sceneYToWidget(static_cast<int>(sceneY));
        if (wy < RenderState::RulerHeight || wy > h) continue;
        p.setPen(penSemitone);
        p.drawLine(RenderState::PianoWidth, wy, w, wy);
    }

    double totalDuration = s.audioDuration > 0 ? s.audioDuration
                         : s.dsFile            ? usToSec(s.dsFile->getTotalDuration())
                                               : 10.0;
    QPen penBar(Colors::BarLine, 1);
    p.setPen(penBar);
    for (int t = 0; t <= static_cast<int>(totalDuration) + 2; ++t) {
        double sceneX = s.timeToX(static_cast<double>(t));
        int wx = s.sceneXToWidget(sceneX);
        if (wx < RenderState::PianoWidth || wx > w) continue;
        p.drawLine(wx, RenderState::RulerHeight, wx, h);
    }

    p.restore();
}

// ============================================================================
// Piano Keys
// ============================================================================

void PianoRollRenderer::drawPianoKeys(QPainter &p, int h, const RenderState &s) {
    p.save();
    p.setClipRect(0, RenderState::RulerHeight, RenderState::PianoWidth,
                  h - RenderState::RulerHeight);
    p.fillRect(0, RenderState::RulerHeight, RenderState::PianoWidth,
               h - RenderState::RulerHeight, Colors::PianoBg);

    static const QSet<int> blackKeys = {1, 3, 6, 8, 10};
    QFont font("Segoe UI", 7);
    p.setFont(font);

    for (int midi = RenderState::MinMidi; midi <= RenderState::MaxMidi; ++midi) {
        double sceneY = s.midiToY(midi + 0.5);
        int wy = s.sceneYToWidget(static_cast<int>(sceneY));
        double sceneYNext = sceneY + s.vScale;
        int wyNext = s.sceneYToWidget(static_cast<int>(sceneYNext));
        int keyH = qMax(1, wyNext - wy);

        if (wy > h || wyNext < RenderState::RulerHeight) continue;

        int noteInOctave = midi % 12;
        if (blackKeys.contains(noteInOctave)) {
            p.setPen(Qt::NoPen);
            p.setBrush(Colors::PianoBlack);
            p.drawRect(0, wy, static_cast<int>(RenderState::PianoWidth * 0.6), keyH);
        } else {
            p.setPen(QPen(QColor("#CCCCCC"), 0.5));
            p.setBrush(Colors::PianoWhite);
            p.drawRect(0, wy, RenderState::PianoWidth, keyH);
            if (noteInOctave == 0) {
                int octave = midi / 12 - 1;
                p.setPen(QColor("#5A5A68"));
                p.drawText(static_cast<int>(RenderState::PianoWidth * 0.6) + 2,
                           wy + qMax(10, static_cast<int>(keyH * 0.6)),
                           QString("C%1").arg(octave));
            }
        }
    }

    p.restore();
}

// ============================================================================
// Ruler
// ============================================================================

void PianoRollRenderer::drawRuler(QPainter &p, int w, const RenderState &s) {
    p.save();
    p.setClipRect(RenderState::PianoWidth, 0, w - RenderState::PianoWidth,
                  RenderState::RulerHeight);
    p.fillRect(RenderState::PianoWidth, 0, w - RenderState::PianoWidth,
               RenderState::RulerHeight, Colors::RulerBg);

    double tLeft = s.xToTime(s.widgetXToScene(RenderState::PianoWidth));
    double tRight = s.xToTime(s.widgetXToScene(w));
    tLeft = qMax(0.0, tLeft);

    const auto &level = findRulerLevel(s.hScale);

    QFont font("Consolas", 9);
    p.setFont(font);

    // Minor ticks
    double t = std::floor(tLeft / level.minorSec) * level.minorSec;
    int tickH = RenderState::RulerHeight;
    while (t <= tRight + level.minorSec) {
        if (t >= 0) {
            double sceneX = s.timeToX(t);
            int wx = s.sceneXToWidget(sceneX);
            bool isMajor = std::fmod(t, level.majorSec) < level.minorSec * 0.5;
            p.setPen(QPen(isMajor ? Colors::RulerText : Colors::RulerTick));
            int hTick = isMajor ? tickH - 6 : tickH - 3;
            p.drawLine(wx, hTick, wx, tickH);
            if (isMajor) {
                p.setPen(Colors::RulerText);
                p.drawText(wx + 2, 14, QString::number(t, 'f', 1));
            }
        }
        t += level.minorSec;
    }

    p.restore();
}

// ============================================================================
// Notes
// ============================================================================

void PianoRollRenderer::drawNotes(QPainter &p, int w, int h, const RenderState &s) {
    if (!s.dsFile) return;

    p.save();
    p.setClipRect(RenderState::PianoWidth, RenderState::RulerHeight,
                  w - RenderState::PianoWidth, h - RenderState::RulerHeight);

    QFont nameFont("Segoe UI", 9, QFont::Bold);
    QFontMetrics fm(nameFont);

    QVector<QPair<QPointF, QString>> noteDescriptions;
    QVector<QPair<QPointF, QString>> phonemeTexts;

    for (int i = 0; i < static_cast<int>(s.dsFile->notes.size()); ++i) {
        const auto &note = s.dsFile->notes[i];

        double sceneX1 = s.timeToX(usToSec(note.start));
        double sceneX2 = s.timeToX(usToSec(note.end()));
        int wx1 = s.sceneXToWidget(sceneX1);
        int wx2 = s.sceneXToWidget(sceneX2);

        if (wx2 < RenderState::PianoWidth || wx1 > w) continue;

        double noteMidi;
        QColor fillColor, borderColor;
        bool isRest = note.isRest();
        bool isSelected = s.selectedNotes && s.selectedNotes->count(i) > 0;

        if (isRest) {
            noteMidi = PitchProcessor::getRestMidi(*s.dsFile, i);
            fillColor = Colors::NoteRestFill;
            borderColor = Colors::NoteRestBorder;
        } else {
            auto pitch = parseNoteName(note.name);
            if (!pitch.valid) continue;
            noteMidi = pitch.midiNumber;
            if (isSelected) {
                fillColor = Colors::NoteSelectedTop;
                borderColor = Colors::NoteSelectedBorder;
            } else if (note.isSlur()) {
                fillColor = Colors::NoteSlur;
                borderColor = Colors::NoteSlurBorder;
            } else {
                fillColor = Colors::NoteDefault;
                borderColor = Colors::NoteBorder;
            }
        }

        double sceneY = s.midiToY(noteMidi + 0.5);
        if (s.draggingNote && isSelected && s.dragAccumulatedCents != 0) {
            double previewMidi = noteMidi + s.dragAccumulatedCents / 100.0;
            sceneY = s.midiToY(previewMidi + 0.5);
        }
        int wy = s.sceneYToWidget(sceneY);
        int noteH = static_cast<int>(s.vScale);

        if (wy + noteH < RenderState::RulerHeight || wy > h) continue;

        int noteW = qMax(2, wx2 - wx1);
        int drawX = qMax(RenderState::PianoWidth, wx1);

        if (!isRest) {
            if (isSelected) {
                p.setPen(Qt::NoPen);
                p.setBrush(Colors::NoteSelectedGlow);
                p.drawRoundedRect(drawX - 3, wy - 3, noteW + 6, noteH + 6, 4, 4);
            }

            QLinearGradient grad(drawX, wy, drawX, wy + noteH);
            if (isSelected) {
                grad.setColorAt(0.0, Colors::NoteSelectedTop);
                grad.setColorAt(0.5, fillColor);
                grad.setColorAt(1.0, Colors::NoteSelectedBottom);
            } else {
                grad.setColorAt(0.0, Colors::NoteDefaultTop);
                grad.setColorAt(0.10, fillColor);
                grad.setColorAt(0.95, fillColor);
                grad.setColorAt(1.0, Colors::NoteDefaultBottom);
            }
            p.setPen(QPen(borderColor, 0));
            p.setBrush(grad);
        } else {
            p.setPen(QPen(borderColor, 0));
            p.setBrush(fillColor);
        }

        p.drawRoundedRect(drawX, wy, noteW, noteH, 2, 2);

        if (!isRest) {
            p.setPen(QPen(borderColor, 0));
            p.drawLine(drawX, wy + noteH / 2, drawX + noteW, wy + noteH / 2);
        }

        if (!isRest) {
            QString noteDescText = note.name;
            if (note.glide == "up") noteDescText.prepend(QStringLiteral("↗"));
            if (note.glide == "down") noteDescText.append(QStringLiteral("↘"));
            noteDescriptions.append({QPointF(drawX, wy - 3), noteDescText});
        }

        if (s.showPhonemeTexts) {
            for (const auto &phone : s.dsFile->phones) {
                if (phone.start + phone.duration <= note.start + 1000) continue;
                if (phone.start >= note.end() - 1000) break;

                double phSceneX1 = s.timeToX(usToSec(phone.start));
                double phSceneX2 = s.timeToX(usToSec(phone.end()));
                int phWx1 = s.sceneXToWidget(phSceneX1);
                int phWx2 = s.sceneXToWidget(phSceneX2);
                int phW = qMax(2, phWx2 - phWx1);
                int phDrawX = qMax(RenderState::PianoWidth, phWx1);

                int phY = wy + noteH;
                int phH = fm.lineSpacing() + 3;

                QPen sepPen(QColor(200, 200, 200, 255));
                sepPen.setWidth(2);
                p.setPen(sepPen);
                p.drawLine(phDrawX + 1, phY + 1, phDrawX + 1, phY + phH);

                p.setPen(Qt::NoPen);
                p.setBrush(QColor(200, 200, 200, 80));
                p.drawRect(phDrawX, phY, phW, phH);
                p.setBrush(Qt::NoBrush);

                phonemeTexts.append({QPointF(phDrawX + 4, phY + fm.ascent()), phone.symbol});
            }
        }
    }

    p.setFont(nameFont);
    p.setPen(Colors::NoteText);
    for (const auto &desc : noteDescriptions) {
        p.drawText(desc.first, desc.second);
    }

    p.setPen(QColor(220, 220, 220));
    for (const auto &ph : phonemeTexts) {
        p.drawText(ph.first, ph.second);
    }

    p.restore();
}

// ============================================================================
// F0 Curve
// ============================================================================

void PianoRollRenderer::drawF0Curve(QPainter &p, int w, int h, const RenderState &s) {
    if (!s.dsFile || s.dsFile->f0.values.empty() || s.dsFile->f0.timestep <= 0)
        return;

    p.save();
    p.setClipRect(RenderState::PianoWidth, RenderState::RulerHeight,
                  w - RenderState::PianoWidth, h - RenderState::RulerHeight);

    const auto &f0 = s.dsFile->f0;
    const auto &values = f0.values;
    double timestepSec = usToSec(f0.timestep);
    double offsetSec = usToSec(s.dsFile->offset);

    auto mhzToMidi = [](int32_t mhz) -> double {
        double hz = dstools::mhzToHz(mhz);
        return (hz > 0.0) ? 12.0 * std::log2(hz / 440.0) + 69.0 : 0.0;
    };

    QPainterPath path;
    bool first = true;

    for (size_t i = 0; i < values.size(); ++i) {
        int32_t val = values[i];
        if (val <= 0) { first = true; continue; }
        double midi = mhzToMidi(val);
        if (std::abs(midi) > 1000) { first = true; continue; }

        double sceneX = s.timeToX(offsetSec + i * timestepSec);
        double sceneY = s.midiToY(midi);
        int wx = s.sceneXToWidget(sceneX);
        int wy = s.sceneYToWidget(sceneY);

        if (first) { path.moveTo(wx, wy); first = false; }
        else path.lineTo(wx, wy);
    }

    if (!path.isEmpty()) {
        if (s.abComparisonActive && s.originalF0 && !s.originalF0->empty()) {
            QPainterPath origPath;
            bool origFirst = true;
            for (size_t i = 0; i < s.originalF0->size(); ++i) {
                int32_t val = (*s.originalF0)[i];
                if (val <= 0) { origFirst = true; continue; }
                double midi = mhzToMidi(val);
                if (std::abs(midi) > 1000) { origFirst = true; continue; }
                double sceneX = s.timeToX(offsetSec + i * timestepSec);
                double sceneY = s.midiToY(midi);
                int wx = s.sceneXToWidget(sceneX);
                int wy = s.sceneYToWidget(sceneY);
                if (origFirst) { origPath.moveTo(wx, wy); origFirst = false; }
                else origPath.lineTo(wx, wy);
            }
            if (!origPath.isEmpty()) {
                QPen origPen(QColor("#4DD0E1"), 2);
                origPen.setStyle(Qt::PenStyle::DashLine);
                p.setPen(origPen);
                p.drawPath(origPath);
            }
        }

        if (s.selectedNotes && !s.selectedNotes->empty() && s.dsFile) {
            p.setPen(QPen(Colors::F0Dimmed, 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);

            for (int noteIdx : *s.selectedNotes) {
                if (noteIdx < 0 || noteIdx >= static_cast<int>(s.dsFile->notes.size())) continue;
                const auto &note = s.dsFile->notes[noteIdx];
                double noteStartSec = usToSec(note.start);
                double noteEndSec = usToSec(note.end());

                QPainterPath selPath;
                bool selFirst = true;
                for (size_t i = 0; i < values.size(); ++i) {
                    double t = offsetSec + i * timestepSec;
                    if (t < noteStartSec || t > noteEndSec) continue;
                    int32_t val = values[i];
                    if (val <= 0) { selFirst = true; continue; }
                    double midi = mhzToMidi(val);
                    if (std::abs(midi) > 1000) { selFirst = true; continue; }
                    double sx = s.timeToX(t);
                    double sy = s.midiToY(midi);
                    int swx = s.sceneXToWidget(sx);
                    int swy = s.sceneYToWidget(sy);
                    if (selFirst) { selPath.moveTo(swx, swy); selFirst = false; }
                    else selPath.lineTo(swx, swy);
                }
                if (!selPath.isEmpty()) {
                    p.setPen(QPen(Colors::F0Selected, 2.5));
                    p.drawPath(selPath);
                }
            }
        } else {
            p.setPen(QPen(Colors::F0Default, 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        }
    }

    p.restore();
}

// ============================================================================
// Playhead
// ============================================================================

void PianoRollRenderer::drawPlayhead(QPainter &p, int w, int h, const RenderState &s) {
    double sceneX = s.timeToX(s.playheadPos);
    int wx = s.sceneXToWidget(sceneX);
    if (wx >= RenderState::PianoWidth && wx < w) {
        p.setPen(QPen(s.isPlaying ? Colors::Playhead : Colors::PlayheadIdle, 2));
        p.drawLine(wx, RenderState::RulerHeight, wx, h);

        p.setBrush(s.isPlaying ? Colors::Playhead : Colors::PlayheadIdle);
        p.setPen(Qt::NoPen);
        QPolygonF triangle;
        triangle << QPointF(wx - 4, 0) << QPointF(wx + 4, 0) << QPointF(wx, 8);
        p.drawPolygon(triangle);
    }
}

// ============================================================================
// Crosshair
// ============================================================================

void PianoRollRenderer::drawCrosshair(QPainter &p, int w, int h, const RenderState &s) {
    if (!s.showCrosshairAndPitch) return;

    int mx = s.mousePos.x();
    int my = s.mousePos.y();

    if (mx < RenderState::PianoWidth || mx > w || my < RenderState::RulerHeight || my > h) return;

    p.save();
    p.setClipRect(RenderState::PianoWidth, RenderState::RulerHeight,
                  w - RenderState::PianoWidth, h - RenderState::RulerHeight);

    QPen crossPen(QColor(200, 200, 200, 100), 1, Qt::DashLine);
    p.setPen(crossPen);
    p.drawLine(mx, RenderState::RulerHeight, mx, h);
    p.drawLine(RenderState::PianoWidth, my, w, my);

    double sceneX = s.widgetXToScene(mx);
    double sceneY = s.widgetYToScene(my);
    double time = s.xToTime(sceneX);
    double midi = s.yToMidi(sceneY);

    QString pitchText = midiToNoteName(static_cast<int>(std::round(midi)));
    if (pitchText.isEmpty()) pitchText = QString::number(midi, 'f', 1);
    QString timeText = QString::number(time, 'f', 3) + "s";
    QString label = pitchText + " | " + timeText;

    QFont font("Segoe UI", 9);
    p.setFont(font);
    QFontMetrics fm(font);
    int textW = fm.horizontalAdvance(label) + 10;
    int textH = fm.height() + 6;

    int textX = mx + 12;
    int textY = my - textH - 4;
    if (textX + textW > w) textX = mx - textW - 4;
    if (textY < RenderState::RulerHeight) textY = my + 8;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 30, 40, 200));
    p.drawRoundedRect(textX, textY, textW, textH, 3, 3);

    p.setPen(QColor(220, 220, 230));
    p.drawText(textX + 5, textY + fm.ascent() + 3, label);

    p.restore();
}

// ============================================================================
// Rubber Band
// ============================================================================

void PianoRollRenderer::drawRubberBand(QPainter &p, const RenderState &s) {
    if (!s.rubberBandActive || s.rubberBandRect.isNull()) return;
    p.save();
    p.setPen(QPen(Colors::RubberBandBorder, 1, Qt::DashLine));
    p.setBrush(Colors::RubberBandFill);
    p.drawRect(s.rubberBandRect);
    p.restore();
}

// ============================================================================
// Snap Guide
// ============================================================================

void PianoRollRenderer::drawSnapGuide(QPainter &p, int w, int h, const RenderState &s) {
    if (!s.draggingNote || !s.selectedNotes || s.selectedNotes->empty() ||
        !s.dsFile || s.dragAccumulatedCents == 0 || !s.dragOrigMidi)
        return;

    p.save();
    p.setClipRect(RenderState::PianoWidth, RenderState::RulerHeight,
                  w - RenderState::PianoWidth, h - RenderState::RulerHeight);

    int firstIdx = *s.selectedNotes->begin();
    if (firstIdx >= static_cast<int>(s.dsFile->notes.size())) { p.restore(); return; }

    auto it = s.dragOrigMidi->find(firstIdx);
    if (it == s.dragOrigMidi->end()) { p.restore(); return; }

    double origMidi = it->second;
    double newMidi = origMidi + s.dragAccumulatedCents / 100.0;
    const auto &note = s.dsFile->notes[firstIdx];

    double x1 = s.timeToX(usToSec(note.start));
    double x2 = s.timeToX(usToSec(note.end()));
    int wx1 = s.sceneXToWidget(x1);
    int wx2 = s.sceneXToWidget(x2);
    int origWy = s.sceneYToWidget(s.midiToY(origMidi));
    int newWy = s.sceneYToWidget(s.midiToY(newMidi));
    int midX = (wx1 + wx2) / 2;

    QPen guidePen(Colors::SnapGuide, 1.5, Qt::DashLine);
    p.setPen(guidePen);
    p.drawLine(wx1, origWy, wx2, origWy);
    QPen vertPen(Colors::SnapGuide, 1.0, Qt::DashLine);
    p.setPen(vertPen);
    p.drawLine(midX, origWy, midX, newWy);
    p.drawLine(wx1, newWy, wx2, newWy);

    QString label = midiToNoteString(origMidi);
    QFont font("Segoe UI", 9, QFont::Bold);
    p.setFont(font);
    p.setPen(Colors::SnapGuide);
    QFontMetrics fm(font);
    p.drawText(wx1 - fm.horizontalAdvance(label) - 4,
               origWy + fm.ascent() / 2, label);

    p.restore();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
