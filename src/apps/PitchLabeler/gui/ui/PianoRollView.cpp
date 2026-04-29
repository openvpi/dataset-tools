#include "PianoRollView.h"
#include "gui/DSFile.h"
#include "commands/PitchCommands.h"

#include "../../PitchLabelerKeys.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QDebug>

#include <dstools/AppSettings.h>

#include <cmath>
#include <algorithm>
#include <map>

namespace dstools {
namespace pitchlabeler {
namespace ui {



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

    // Selection colors (matching Python reference)
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



// ============================================================================
// PianoRollView
// ============================================================================

PianoRollView::PianoRollView(QWidget *parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setMinimumSize(400, 300);
    setMouseTracking(true); // Enable crosshair tracking
    setFocusPolicy(Qt::StrongFocus); // Receive keyboard events

    m_hScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_vScrollBar = new QScrollBar(Qt::Vertical, this);

    connect(m_hScrollBar, &QScrollBar::valueChanged, this, [this](int val) {
        // Convert pixel scroll to time and update viewport
        double startSec = static_cast<double>(val) / m_hScale;
        double drawW = width() - ScrollBarSize - PianoWidth;
        double endSec = startSec + drawW / m_hScale;
        m_viewport->setViewRange(startSec, endSec);
    });
    connect(m_vScrollBar, &QScrollBar::valueChanged, this, [this](int val) {
        m_scrollY = val;
        update();
    });

    buildContextMenu();
    buildNoteMenu();

    // Center view on middle C area by default
    m_scrollY = static_cast<int>((MaxMidi - 72) * m_vScale);
}

PianoRollView::~PianoRollView() = default;

void PianoRollView::restoreToolCursor() {
    switch (m_toolMode) {
        case ToolSelect:
            setCursor(Qt::ArrowCursor);
            break;
        case ToolModulation:
        case ToolDrift:
            setCursor(Qt::SizeVerCursor);
            break;
    }
}

void PianoRollView::buildContextMenu() {
    m_bgMenu = new QMenu(this);

    auto *zoomIn = new QAction(QStringLiteral("放大"), m_bgMenu);
    zoomIn->setShortcut(QKeySequence("Ctrl+="));
    connect(zoomIn, &QAction::triggered, this, &PianoRollView::zoomIn);
    m_bgMenu->addAction(zoomIn);

    auto *zoomOut = new QAction(QStringLiteral("缩小"), m_bgMenu);
    zoomOut->setShortcut(QKeySequence("Ctrl+-"));
    connect(zoomOut, &QAction::triggered, this, &PianoRollView::zoomOut);
    m_bgMenu->addAction(zoomOut);

    m_bgMenu->addSeparator();

    auto *reset = new QAction(QStringLiteral("重置视图"), m_bgMenu);
    reset->setShortcut(QKeySequence("Ctrl+0"));
    connect(reset, &QAction::triggered, this, &PianoRollView::resetZoom);
    m_bgMenu->addAction(reset);

    m_bgMenu->addSeparator();

    // Display options (following SlurCutter F0Widget pattern)
    auto *optionsPrompt = new QAction(QStringLiteral("显示选项"), m_bgMenu);
    auto boldFont = optionsPrompt->font();
    boldFont.setBold(true);
    optionsPrompt->setFont(boldFont);
    optionsPrompt->setEnabled(false);
    m_bgMenu->addAction(optionsPrompt);

    m_actSnapToKey = new QAction(QStringLiteral("默认吸附到琴键(&S)"), m_bgMenu);
    m_actSnapToKey->setCheckable(true);
    m_actSnapToKey->setChecked(m_snapToKey);
    connect(m_actSnapToKey, &QAction::triggered, this, [this] {
        m_snapToKey = m_actSnapToKey->isChecked();
        update();
    });
    m_bgMenu->addAction(m_actSnapToKey);

    m_actShowPitchTextOverlay = new QAction(QStringLiteral("显示音高文字覆盖(&O)"), m_bgMenu);
    m_actShowPitchTextOverlay->setCheckable(true);
    m_actShowPitchTextOverlay->setChecked(m_showPitchTextOverlay);
    connect(m_actShowPitchTextOverlay, &QAction::triggered, this, [this] {
        m_showPitchTextOverlay = m_actShowPitchTextOverlay->isChecked();
        update();
    });
    m_bgMenu->addAction(m_actShowPitchTextOverlay);

    m_actShowPhonemeTexts = new QAction(QStringLiteral("显示音素(&P)"), m_bgMenu);
    m_actShowPhonemeTexts->setCheckable(true);
    m_actShowPhonemeTexts->setChecked(m_showPhonemeTexts);
    connect(m_actShowPhonemeTexts, &QAction::triggered, this, [this] {
        m_showPhonemeTexts = m_actShowPhonemeTexts->isChecked();
        update();
    });
    m_bgMenu->addAction(m_actShowPhonemeTexts);

    m_actShowCrosshairAndPitch = new QAction(QStringLiteral("显示十字光标和音高(&C)"), m_bgMenu);
    m_actShowCrosshairAndPitch->setCheckable(true);
    m_actShowCrosshairAndPitch->setChecked(m_showCrosshairAndPitch);
    connect(m_actShowCrosshairAndPitch, &QAction::triggered, this, [this] {
        m_showCrosshairAndPitch = m_actShowCrosshairAndPitch->isChecked();
        update();
    });
    m_bgMenu->addAction(m_actShowCrosshairAndPitch);
}

void PianoRollView::buildNoteMenu() {
    m_noteMenu = new QMenu(this);

    auto boldFont = m_noteMenu->font();
    boldFont.setBold(true);

    m_actMergeLeft = new QAction(QStringLiteral("合并到左侧(&M)"), m_noteMenu);
    connect(m_actMergeLeft, &QAction::triggered, this, [this] {
        if (m_contextNoteIndex >= 0) emit noteMergeLeft(m_contextNoteIndex);
    });
    m_noteMenu->addAction(m_actMergeLeft);

    m_actToggleRest = new QAction(QStringLiteral("切换休止(&R)"), m_noteMenu);
    connect(m_actToggleRest, &QAction::triggered, this, [this] {
        if (m_contextNoteIndex >= 0) emit noteRestToggled(m_contextNoteIndex);
    });
    m_noteMenu->addAction(m_actToggleRest);

    m_actToggleSlur = new QAction(QStringLiteral("切换连音(&L)"), m_noteMenu);
    connect(m_actToggleSlur, &QAction::triggered, this, [this] {
        if (m_contextNoteIndex >= 0) emit noteSlurToggled(m_contextNoteIndex);
    });
    m_noteMenu->addAction(m_actToggleSlur);

    m_noteMenu->addSeparator();

    // Glide sub-section
    auto *glidePrompt = new QAction(QStringLiteral("装饰音: 滑音"), m_noteMenu);
    glidePrompt->setFont(boldFont);
    glidePrompt->setEnabled(false);
    m_noteMenu->addAction(glidePrompt);

    m_actGlideGroup = new QActionGroup(this);
    m_actGlideNone = new QAction(QStringLiteral("无"), m_noteMenu);
    m_actGlideUp = new QAction(QStringLiteral("上滑"), m_noteMenu);
    m_actGlideDown = new QAction(QStringLiteral("下滑"), m_noteMenu);
    m_actGlideNone->setCheckable(true);
    m_actGlideUp->setCheckable(true);
    m_actGlideDown->setCheckable(true);
    m_actGlideNone->setChecked(true);
    m_actGlideGroup->addAction(m_actGlideNone);
    m_actGlideGroup->addAction(m_actGlideUp);
    m_actGlideGroup->addAction(m_actGlideDown);
    m_noteMenu->addAction(m_actGlideNone);
    m_noteMenu->addAction(m_actGlideUp);
    m_noteMenu->addAction(m_actGlideDown);

    connect(m_actGlideGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        if (m_contextNoteIndex < 0) return;
        QString glide = "none";
        if (action == m_actGlideUp) glide = "up";
        else if (action == m_actGlideDown) glide = "down";
        emit noteGlideChanged(m_contextNoteIndex, glide);
    });
}

void PianoRollView::updateNoteMenuState() {
    if (!m_dsFile || m_contextNoteIndex < 0 ||
        m_contextNoteIndex >= static_cast<int>(m_dsFile->notes.size()))
        return;

    const auto &note = m_dsFile->notes[m_contextNoteIndex];

    // Merge left only possible for slur notes with index > 0
    m_actMergeLeft->setEnabled(note.isSlur() && m_contextNoteIndex > 0);

    // Toggle rest label
    m_actToggleRest->setText(note.isRest()
        ? QStringLiteral("转为普通音符(&R)")
        : QStringLiteral("标记为休止(&R)"));

    // Toggle slur label
    m_actToggleSlur->setText(note.isSlur()
        ? QStringLiteral("取消连音(&L)")
        : QStringLiteral("设为连音(&L)"));

    // Glide state
    if (note.glide == "up") m_actGlideUp->setChecked(true);
    else if (note.glide == "down") m_actGlideDown->setChecked(true);
    else m_actGlideNone->setChecked(true);
}

void PianoRollView::setDSFile(std::shared_ptr<pitchlabeler::DSFile> ds) {
    m_dsFile = ds;
    m_selectedNotes.clear();
    m_draggingNote = false;
    m_modulationDragging = false;
    m_driftDragging = false;
    m_rubberBandActive = false;
    m_preAdjustF0.clear();

    // Store original F0 for A/B comparison
    if (ds && !ds->f0.values.empty()) {
        m_originalF0 = ds->f0.values;
    } else {
        m_originalF0.clear();
    }

    // Auto-center on note range
    if (ds && !ds->notes.empty()) {
        double minMidi = 127, maxMidi = 0;
        for (const auto &note : ds->notes) {
            if (note.isRest()) continue;
            auto pitch = parseNoteName(note.name);
            if (pitch.valid) {
                minMidi = std::min(minMidi, pitch.midiNumber);
                maxMidi = std::max(maxMidi, pitch.midiNumber);
            }
        }
        if (maxMidi > minMidi) {
            double centerMidi = (minMidi + maxMidi) / 2.0;
            int drawH = height() - RulerHeight - ScrollBarSize;
            m_scrollY = std::max(0.0, midiToY(centerMidi) - drawH / 2.0);
        }
    }

    updateScrollBars();
    update();
}

void PianoRollView::clear() {
    m_dsFile.reset();
    m_audioDuration = 0.0;
    m_selectedNotes.clear();
    m_draggingNote = false;
    m_modulationDragging = false;
    m_driftDragging = false;
    m_rubberBandActive = false;
    m_preAdjustF0.clear();
    update();
}

void PianoRollView::setAudioDuration(double sec) {
    m_audioDuration = sec;
    if (m_viewport) {
        m_viewport->setTotalDuration(sec);
    }
    if (m_audioDuration > 0) {
        double drawW = width() - ScrollBarSize;
        double minScale = (drawW - PianoWidth) / m_audioDuration;
        if (m_hScale < minScale) {
            m_viewport->setPixelsPerSecond(minScale);
        }
    }
    updateScrollBars();
    update();
}

void PianoRollView::zoomIn() {
    m_viewport->zoomAt(m_viewport->viewCenter(), 1.2);
}

void PianoRollView::zoomOut() {
    double minScale = 20.0;
    if (m_audioDuration > 0) {
        double drawW = width() - ScrollBarSize;
        minScale = qMax(minScale, (drawW - PianoWidth) / m_audioDuration);
    }
    double newScale = qMax(m_hScale / 1.2, minScale);
    m_viewport->zoomAt(m_viewport->viewCenter(), newScale / m_hScale);
}

void PianoRollView::resetZoom() {
    double newScale = 100.0;
    if (m_audioDuration > 0) {
        double drawW = width() - ScrollBarSize;
        double minScale = (drawW - PianoWidth) / m_audioDuration;
        newScale = qMax(newScale, minScale);
    }
    m_viewport->setPixelsPerSecond(newScale);
    m_vScale = 20.0;
    updateScrollBars();
    update();
}

int PianoRollView::getZoomPercent() const {
    return static_cast<int>(m_hScale / 100.0 * 100.0);
}

void PianoRollView::setPlayheadTime(double sec) {
    m_playheadPos = sec;
    update();
}

void PianoRollView::setPlayheadState(bool playing) {
    m_isPlaying = playing;
    update();
}

void PianoRollView::setABComparisonActive(bool active) {
    m_abComparisonActive = active;
    update();
}

void PianoRollView::storeOriginalF0(const std::vector<double> &original) {
    m_originalF0 = original;
}

void PianoRollView::setToolMode(ToolMode mode) {
    if (m_toolMode != mode) {
        m_toolMode = mode;
        // Update cursor to reflect tool mode (matching Python reference behavior)
        switch (mode) {
            case ToolSelect:
                setCursor(Qt::ArrowCursor);
                break;
            case ToolModulation:
            case ToolDrift:
                setCursor(Qt::SizeVerCursor);
                break;
        }
        emit toolModeChanged(static_cast<int>(mode));
        update();
    }
}

// ============================================================================
// Coordinate conversion
// ============================================================================

// Scene coordinates: time -> X, midi -> Y (higher pitch = lower Y)
// These match the reference's time_to_x / midi_to_y

double PianoRollView::timeToX(double time) const {
    return time * m_hScale + PianoWidth;
}

double PianoRollView::xToTime(double x) const {
    return (x - PianoWidth) / m_hScale;
}

double PianoRollView::midiToY(double midi) const {
    return (MaxMidi - midi) * m_vScale + RulerHeight;
}

double PianoRollView::yToMidi(double y) const {
    return MaxMidi - (y - RulerHeight) / m_vScale;
}

// Scene to widget (subtract scroll offset)
int PianoRollView::sceneXToWidget(double sceneX) const {
    return static_cast<int>(sceneX - m_scrollX);
}

int PianoRollView::sceneYToWidget(double sceneY) const {
    return static_cast<int>(sceneY - m_scrollY);
}

double PianoRollView::widgetXToScene(int wx) const {
    return wx + m_scrollX;
}

double PianoRollView::widgetYToScene(int wy) const {
    return wy + m_scrollY;
}

// ============================================================================
// Scroll bars
// ============================================================================

void PianoRollView::updateScrollBars() {
    int drawW = width() - ScrollBarSize;
    int drawH = height() - ScrollBarSize;

    // Total scene size — limited to audio file duration when available
    double totalDuration = m_audioDuration > 0 ? m_audioDuration
                         : m_dsFile            ? m_dsFile->getTotalDuration()
                                               : 10.0;
    double sceneW = timeToX(totalDuration);
    double sceneH = midiToY(MinMidi) + 50;

    m_hScrollBar->setRange(0, qMax(0, static_cast<int>(sceneW - drawW)));
    m_hScrollBar->setPageStep(drawW);

    m_vScrollBar->setRange(0, qMax(0, static_cast<int>(sceneH - drawH)));
    m_vScrollBar->setPageStep(drawH);
}

// ============================================================================
// Note hit testing
// ============================================================================

double PianoRollView::getRestMidi(int index) const {
    if (!m_dsFile) return 60.0;
    const auto &notes = m_dsFile->notes;
    for (int offset = 1; offset < static_cast<int>(notes.size()); ++offset) {
        for (int idx : {index - offset, index + offset}) {
            if (idx >= 0 && idx < static_cast<int>(notes.size())) {
                const auto &n = notes[idx];
                if (!n.isRest()) {
                    auto pitch = parseNoteName(n.name);
                    if (pitch.valid) return pitch.midiNumber;
                }
            }
        }
    }
    return 60.0;
}

int PianoRollView::getNoteAtPosition(int x, int y) const {
    if (!m_dsFile) return -1;

    double sceneX = widgetXToScene(x);
    double sceneY = widgetYToScene(y);
    double time = xToTime(sceneX);
    double midi = yToMidi(sceneY);

    for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i) {
        const auto &note = m_dsFile->notes[i];
        if (time >= note.start && time < note.end()) {
            if (note.isRest()) {
                double restMidi = getRestMidi(i);
                if (std::abs(midi - restMidi) < 1.0) return i;
            } else {
                auto pitch = parseNoteName(note.name);
                if (pitch.valid && std::abs(midi - pitch.midiNumber) < 1.0) {
                    return i;
                }
            }
        }
    }
    return -1;
}

// ============================================================================
// Drawing
// ============================================================================

void PianoRollView::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width() - ScrollBarSize;
    int h = height() - ScrollBarSize;

    // Background
    p.fillRect(0, 0, w, h, Colors::Background);

    // Clip to drawing area (excluding scrollbars)
    p.setClipRect(0, 0, w, h);

    // Draw grid (semitone lines + bar lines)
    drawGrid(p, w, h);

    // Draw notes (bottom layer)
    drawNotes(p, w, h);

    // Draw F0 curve (above notes — pitch curve should overlay notes for visibility)
    drawF0Curve(p, w, h);

    // Draw playhead
    drawPlayhead(p, w, h);

    // Draw crosshair (before overlays, after content)
    drawCrosshair(p, w, h);

    // Draw snap guide (during pitch drag)
    drawSnapGuide(p, w, h);

    // Draw rubber band selection
    drawRubberBand(p);

    // Draw foreground overlays (ruler + piano keys) - on top of everything
    drawRuler(p, w);
    drawPianoKeys(p, h);

    // Corner patch (ruler x piano intersection)
    p.setClipRect(0, 0, w, h);
    p.fillRect(0, 0, PianoWidth, RulerHeight, Colors::RulerBg);

    // Position scroll bars
    m_hScrollBar->setGeometry(0, h, w, ScrollBarSize);
    m_vScrollBar->setGeometry(w, 0, ScrollBarSize, h);
}

void PianoRollView::drawGrid(QPainter &p, int w, int h) {
    p.save();
    p.setClipRect(PianoWidth, RulerHeight, w - PianoWidth, h - RulerHeight);

    // Horizontal semitone lines
    QPen penSemitone(Colors::GridSemitone, 1);
    for (int midi = MinMidi; midi <= MaxMidi; ++midi) {
        double sceneY = midiToY(midi + 0.5);
        int wy = sceneYToWidget(static_cast<int>(sceneY));
        if (wy < RulerHeight || wy > h) continue;

        p.setPen(penSemitone);
        p.drawLine(PianoWidth, wy, w, wy);
    }

    // Vertical bar lines (every second)
    double totalDuration = m_audioDuration > 0 ? m_audioDuration
                         : m_dsFile            ? m_dsFile->getTotalDuration()
                                               : 10.0;
    QPen penBar(Colors::BarLine, 1);
    p.setPen(penBar);
    for (int t = 0; t <= static_cast<int>(totalDuration) + 2; ++t) {
        double sceneX = timeToX(static_cast<double>(t));
        int wx = sceneXToWidget(sceneX);
        if (wx < PianoWidth || wx > w) continue;
        p.drawLine(wx, RulerHeight, wx, h);
    }

    p.restore();
}

void PianoRollView::drawPianoKeys(QPainter &p, int h) {
    p.save();
    p.setClipRect(0, RulerHeight, PianoWidth, h - RulerHeight);
    p.fillRect(0, RulerHeight, PianoWidth, h - RulerHeight, Colors::PianoBg);

    static const QSet<int> blackKeys = {1, 3, 6, 8, 10};
    QFont font("Segoe UI", 7);
    p.setFont(font);

    for (int midi = MinMidi; midi <= MaxMidi; ++midi) {
        double sceneY = midiToY(midi + 0.5);
        int wy = sceneYToWidget(static_cast<int>(sceneY));
        double sceneYNext = sceneY + m_vScale;
        int wyNext = sceneYToWidget(static_cast<int>(sceneYNext));
        int keyH = qMax(1, wyNext - wy);

        if (wy > h || wyNext < RulerHeight) continue;

        int noteInOctave = midi % 12;
        if (blackKeys.contains(noteInOctave)) {
            p.setPen(Qt::NoPen);
            p.setBrush(Colors::PianoBlack);
            p.drawRect(0, wy, static_cast<int>(PianoWidth * 0.6), keyH);
        } else {
            p.setPen(QPen(QColor("#CCCCCC"), 0.5));
            p.setBrush(Colors::PianoWhite);
            p.drawRect(0, wy, PianoWidth, keyH);
            if (noteInOctave == 0) {
                int octave = midi / 12 - 1;
                p.setPen(QColor("#5A5A68"));
                p.drawText(static_cast<int>(PianoWidth * 0.6) + 2,
                           wy + qMax(10, static_cast<int>(keyH * 0.6)),
                           QString("C%1").arg(octave));
            }
        }
    }

    p.restore();
}

void PianoRollView::drawRuler(QPainter &p, int w) {
    p.save();
    p.setClipRect(PianoWidth, 0, w - PianoWidth, RulerHeight);
    p.fillRect(PianoWidth, 0, w - PianoWidth, RulerHeight, Colors::RulerBg);

    // Determine visible time range
    double tLeft = xToTime(widgetXToScene(PianoWidth));
    double tRight = xToTime(widgetXToScene(w));
    tLeft = qMax(0.0, tLeft);

    double interval = (m_hScale > 200) ? 0.5 : (m_hScale > 50 ? 1.0 : 5.0);

    QFont font("Consolas", 9);
    p.setFont(font);

    double t = std::floor(tLeft / interval) * interval;
    while (t <= tRight + interval) {
        if (t >= 0) {
            double sceneX = timeToX(t);
            int wx = sceneXToWidget(sceneX);
            p.setPen(QPen(Colors::RulerTick));
            p.drawLine(wx, RulerHeight - 6, wx, RulerHeight);
            p.setPen(Colors::RulerText);
            p.drawText(wx + 2, 14, QString::number(t, 'f', 1));
        }
        t += interval;
    }

    p.restore();
}

void PianoRollView::drawNotes(QPainter &p, int w, int h) {
    if (!m_dsFile) return;

    p.save();
    p.setClipRect(PianoWidth, RulerHeight, w - PianoWidth, h - RulerHeight);

    QFont nameFont("Segoe UI", 9, QFont::Bold);
    QFontMetrics fm(nameFont);

    // Collect note descriptions and phoneme texts for deferred drawing (like SlurCutter)
    QVector<QPair<QPointF, QString>> noteDescriptions;
    QVector<QPair<QPointF, QString>> phonemeTexts;

    int phoneIdx = 0; // Track current phone index for matching to notes

    for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i) {
        const auto &note = m_dsFile->notes[i];

        double sceneX1 = timeToX(note.start);
        double sceneX2 = timeToX(note.end());
        int wx1 = sceneXToWidget(sceneX1);
        int wx2 = sceneXToWidget(sceneX2);

        // Cull off-screen
        if (wx2 < PianoWidth || wx1 > w) continue;

        double noteMidi;
        QColor fillColor, borderColor;
        bool isRest = note.isRest();
        bool isSelected = m_selectedNotes.count(i) > 0;

        if (isRest) {
            noteMidi = getRestMidi(i);
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

        double sceneY = midiToY(noteMidi + 0.5);
        // During pitch drag, visually offset selected notes
        if (m_draggingNote && isSelected && m_dragAccumulatedCents != 0) {
            double previewMidi = noteMidi + m_dragAccumulatedCents / 100.0;
            sceneY = midiToY(previewMidi + 0.5);
        }
        int wy = sceneYToWidget(sceneY);
        int noteH = static_cast<int>(m_vScale);

        // Cull vertically
        if (wy + noteH < RulerHeight || wy > h) continue;

        int noteW = qMax(2, wx2 - wx1);
        int drawX = qMax(PianoWidth, wx1);

        // Draw gradient fill for non-rest notes
        if (!isRest) {
            // Selection glow
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

        // Center line (like SlurCutter)
        if (!isRest) {
            p.setPen(QPen(borderColor, 0));
            p.drawLine(drawX, wy + noteH / 2, drawX + noteW, wy + noteH / 2);
        }

        // Note description text - draw ABOVE note rect (like SlurCutter)
        if (!isRest) {
            QString noteDescText = note.name;
            if (note.glide == "up") noteDescText.prepend(QStringLiteral("↗"));
            if (note.glide == "down") noteDescText.append(QStringLiteral("↘"));
            noteDescriptions.append({QPointF(drawX, wy - 3), noteDescText});
        }

        // Phoneme display - find phones that fall within this note's time range
        if (m_showPhonemeTexts) {
        for (const auto &phone : m_dsFile->phones) {
            if (phone.start + phone.duration <= note.start + 0.001) continue;
            if (phone.start >= note.end() - 0.001) break;

            double phSceneX1 = timeToX(phone.start);
            double phSceneX2 = timeToX(phone.end());
            int phWx1 = sceneXToWidget(phSceneX1);
            int phWx2 = sceneXToWidget(phSceneX2);
            int phW = qMax(2, phWx2 - phWx1);
            int phDrawX = qMax(PianoWidth, phWx1);

            // Draw phoneme below the note (like SlurCutter)
            int phY = wy + noteH;
            int phH = fm.lineSpacing() + 3;

            // Separator line
            QPen sepPen(QColor(200, 200, 200, 255));
            sepPen.setWidth(2);
            p.setPen(sepPen);
            p.drawLine(phDrawX + 1, phY + 1, phDrawX + 1, phY + phH);

            // Semi-transparent background
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(200, 200, 200, 80));
            p.drawRect(phDrawX, phY, phW, phH);
            p.setBrush(Qt::NoBrush);

            // Phoneme text (deferred)
            phonemeTexts.append({QPointF(phDrawX + 4, phY + fm.ascent()), phone.symbol});
        }
        } // if (m_showPhonemeTexts)
    }

    // Draw deferred note descriptions (on top, to avoid overlap from adjacent notes)
    p.setFont(nameFont);
    p.setPen(Colors::NoteText);
    for (const auto &desc : noteDescriptions) {
        p.drawText(desc.first, desc.second);
    }

    // Draw deferred phoneme texts
    p.setPen(QColor(220, 220, 220));
    for (const auto &ph : phonemeTexts) {
        p.drawText(ph.first, ph.second);
    }

    p.restore();
}

void PianoRollView::drawF0Curve(QPainter &p, int w, int h) {
    if (!m_dsFile || m_dsFile->f0.values.empty() || m_dsFile->f0.timestep <= 0)
        return;

    p.save();
    p.setClipRect(PianoWidth, RulerHeight, w - PianoWidth, h - RulerHeight);

    const auto &f0 = m_dsFile->f0;
    const auto &values = f0.values;
    double timestep = f0.timestep;
    double offset = m_dsFile->offset;

    // Build F0 path
    QPainterPath path;
    bool first = true;

    for (size_t i = 0; i < values.size(); ++i) {
        double val = values[i];
        // Skip unvoiced/invalid samples (f0 values are already MIDI note numbers)
        if (val <= 0 || std::isnan(val) || std::isinf(val)) {
            first = true;
            continue;
        }

        double midi = val;
        if (std::abs(midi) > 1000) {
            first = true;
            continue;
        }

        double sceneX = timeToX(offset + i * timestep);
        double sceneY = midiToY(midi);
        int wx = sceneXToWidget(sceneX);
        int wy = sceneYToWidget(sceneY);

        if (first) {
            path.moveTo(wx, wy);
            first = false;
        } else {
            path.lineTo(wx, wy);
        }
    }

    if (!path.isEmpty()) {
        // Draw A/B comparison: original F0 curve FIRST (below current F0)
        if (m_abComparisonActive && !m_originalF0.empty()) {
            QPainterPath origPath;
            bool origFirst = true;

            for (size_t i = 0; i < m_originalF0.size(); ++i) {
                double val = m_originalF0[i];
                if (val <= 0 || std::isnan(val) || std::isinf(val)) {
                    origFirst = true;
                    continue;
                }

                double midi = val;
                if (std::abs(midi) > 1000) {
                    origFirst = true;
                    continue;
                }

                double sceneX = timeToX(offset + i * timestep);
                double sceneY = midiToY(midi);
                int wx = sceneXToWidget(sceneX);
                int wy = sceneYToWidget(sceneY);

                if (origFirst) {
                    origPath.moveTo(wx, wy);
                    origFirst = false;
                } else {
                    origPath.lineTo(wx, wy);
                }
            }

            if (!origPath.isEmpty()) {
                // Original F0 in blue/cyan, dashed, below current
                QPen origPen(QColor("#4DD0E1"), 2);
                origPen.setStyle(Qt::PenStyle::DashLine);
                p.setPen(origPen);
                p.drawPath(origPath);
            }
        }

        // Draw current F0 curve ON TOP (orange)
        // If we have selected notes, draw dimmed first, then bright for selected segments
        if (!m_selectedNotes.empty() && m_dsFile) {
            // Dimmed full curve
            p.setPen(QPen(Colors::F0Dimmed, 2));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);

            // Bright segments for selected notes
            for (int noteIdx : m_selectedNotes) {
                if (noteIdx < 0 || noteIdx >= static_cast<int>(m_dsFile->notes.size())) continue;
                const auto &note = m_dsFile->notes[noteIdx];
                double noteStart = note.start;
                double noteEnd = note.end();

                QPainterPath selPath;
                bool selFirst = true;
                for (size_t i = 0; i < values.size(); ++i) {
                    double t = offset + i * timestep;
                    if (t < noteStart || t > noteEnd) continue;
                    double val = values[i];
                    if (val <= 0 || std::isnan(val) || std::isinf(val) || std::abs(val) > 1000) {
                        selFirst = true;
                        continue;
                    }
                    double sx = timeToX(t);
                    double sy = midiToY(val);
                    int swx = sceneXToWidget(sx);
                    int swy = sceneYToWidget(sy);
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

void PianoRollView::drawPlayhead(QPainter &p, int w, int h) {
    double sceneX = timeToX(m_playheadPos);
    int wx = sceneXToWidget(sceneX);
    if (wx >= PianoWidth && wx < w) {
        p.setPen(QPen(m_isPlaying ? Colors::Playhead : Colors::PlayheadIdle, 2));
        p.drawLine(wx, RulerHeight, wx, h);

        // Playhead triangle marker in ruler
        p.setBrush(m_isPlaying ? Colors::Playhead : Colors::PlayheadIdle);
        p.setPen(Qt::NoPen);
        QPolygonF triangle;
        triangle << QPointF(wx - 4, 0) << QPointF(wx + 4, 0) << QPointF(wx, 8);
        p.drawPolygon(triangle);
    }
}

void PianoRollView::drawCrosshair(QPainter &p, int w, int h) {
    if (!m_showCrosshairAndPitch) return;
    if (!rect().contains(m_mousePos)) return;

    int mx = m_mousePos.x();
    int my = m_mousePos.y();

    // Only draw in the piano roll area
    if (mx < PianoWidth || mx > w || my < RulerHeight || my > h) return;

    p.save();
    p.setClipRect(PianoWidth, RulerHeight, w - PianoWidth, h - RulerHeight);

    QPen crossPen(QColor(200, 200, 200, 100), 1, Qt::DashLine);
    p.setPen(crossPen);
    p.drawLine(mx, RulerHeight, mx, h);
    p.drawLine(PianoWidth, my, w, my);

    // Show pitch and time text near cursor
    double sceneX = widgetXToScene(mx);
    double sceneY = widgetYToScene(my);
    double time = xToTime(sceneX);
    double midi = yToMidi(sceneY);

    QString pitchText = midiToNoteName(static_cast<int>(std::round(midi)));
    if (pitchText.isEmpty()) pitchText = QString::number(midi, 'f', 1);
    QString timeText = QString::number(time, 'f', 3) + "s";
    QString label = pitchText + " | " + timeText;

    QFont font("Segoe UI", 9);
    p.setFont(font);
    QFontMetrics fm(font);
    int textW = fm.horizontalAdvance(label) + 10;
    int textH = fm.height() + 6;

    // Position text box near cursor, avoid going off-screen
    int textX = mx + 12;
    int textY = my - textH - 4;
    if (textX + textW > w) textX = mx - textW - 4;
    if (textY < RulerHeight) textY = my + 8;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(30, 30, 40, 200));
    p.drawRoundedRect(textX, textY, textW, textH, 3, 3);

    p.setPen(QColor(220, 220, 230));
    p.drawText(textX + 5, textY + fm.ascent() + 3, label);

    p.restore();
}

// ============================================================================
// Events
// ============================================================================

void PianoRollView::resizeEvent(QResizeEvent *event) {
    QFrame::resizeEvent(event);
    if (m_audioDuration > 0) {
        double drawW = width() - ScrollBarSize;
        double minScale = (drawW - PianoWidth) / m_audioDuration;
        if (m_hScale < minScale) {
            m_viewport->setPixelsPerSecond(minScale);
        }
    }
    updateScrollBars();
}

void PianoRollView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom horizontally
        double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
        // Zoom centered on mouse position
        double sceneX = widgetXToScene(static_cast<int>(event->position().x()));
        double centerSec = xToTime(sceneX);
        m_viewport->zoomAt(centerSec, factor);
    } else if (event->modifiers() & Qt::ShiftModifier) {
        // Horizontal scroll
        double deltaSec = -event->angleDelta().y() / m_hScale;
        m_viewport->scrollBy(deltaSec);
    } else {
        // Vertical scroll
        m_vScrollBar->setValue(m_vScrollBar->value() - event->angleDelta().y());
    }
    event->accept();
}

void PianoRollView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        // Middle-button: start pan
        m_isDragging = true;
        m_dragStart = event->pos();
        m_dragButton = event->button();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // Alt+Left: start pan
    if (event->button() == Qt::LeftButton &&
        (event->modifiers() & Qt::AltModifier)) {
        m_isDragging = true;
        m_dragStart = event->pos();
        m_dragButton = Qt::MiddleButton; // treat as pan
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();

        // Click on ruler: seek playhead to clicked time
        if (y < RulerHeight) {
            double sceneX = widgetXToScene(x);
            double time = xToTime(sceneX);
            if (time >= 0) {
                m_playheadPos = time;
                m_rulerDragging = true;
                emit rulerClicked(time);
                update();
            }
            return;
        }
        // Click on piano sidebar: ignore
        if (x < PianoWidth) return;

        int noteIdx = getNoteAtPosition(x, y);

        if (noteIdx >= 0) {
            // Ctrl+click: toggle selection
            if (event->modifiers() & Qt::ControlModifier) {
                auto newSel = m_selectedNotes;
                if (newSel.count(noteIdx))
                    newSel.erase(noteIdx);
                else
                    newSel.insert(noteIdx);
                selectNotes(newSel);
            } else if (m_selectedNotes.count(noteIdx) == 0) {
                // Click on unselected note: select only this one
                selectNotes({noteIdx});
            }
            emit noteSelected(noteIdx);

            // Modulation/Drift tool: start F0 drag
            if (m_toolMode != ToolSelect && m_selectedNotes.count(noteIdx)) {
                double sceneY = widgetYToScene(y);
                // Python reference initializes both start_y values on press
                m_modulationDragStartY = sceneY;
                m_driftDragStartY = sceneY;
                m_modulationDragAmount = 1.0;
                m_driftDragAmount = 1.0;
                if (m_toolMode == ToolModulation) {
                    m_modulationDragging = true;
                } else {
                    m_driftDragging = true;
                }
                if (m_dsFile) {
                    m_preAdjustF0 = m_dsFile->f0.values;
                }
                setCursor(Qt::SizeVerCursor);
                return;
            }

            // Select tool: start pitch drag if note is selected
            if (m_selectedNotes.count(noteIdx) &&
                !(event->modifiers() & Qt::ControlModifier)) {
                m_draggingNote = true;
                double sceneY = widgetYToScene(y);
                m_dragStartMidi = yToMidi(sceneY);
                m_dragAccumulatedCents = 0;
                m_dragOrigMidi.clear();
                if (m_dsFile) {
                    for (int idx : m_selectedNotes) {
                        if (idx < static_cast<int>(m_dsFile->notes.size())) {
                            const auto &n = m_dsFile->notes[idx];
                            if (!n.isRest()) {
                                auto pitch = parseNoteName(n.name);
                                if (pitch.valid)
                                    m_dragOrigMidi[idx] = pitch.midiNumber;
                            }
                        }
                    }
                }
                setCursor(Qt::SizeVerCursor);
            }
        } else {
            // Click on empty space
            if (event->modifiers() & Qt::ControlModifier) {
                // Start rubber-band selection
                m_rubberBandActive = true;
                m_rubberBandStart = event->pos();
                m_rubberBandRect = QRect();
                setCursor(Qt::CrossCursor);
            } else {
                // Deselect all
                selectNotes({});
                double sceneX = widgetXToScene(x);
                double sceneY = widgetYToScene(y);
                emit positionClicked(xToTime(sceneX), yToMidi(sceneY));
            }
        }
    }
}

void PianoRollView::mouseMoveEvent(QMouseEvent *event) {
    m_mousePos = event->pos();

    // Ruler scrub: drag on ruler to scrub playhead
    if (m_rulerDragging) {
        double sceneX = widgetXToScene(event->pos().x());
        double time = std::max(0.0, xToTime(sceneX));
        m_playheadPos = time;
        emit rulerClicked(time);
        update();
        return;
    }

    double sceneY = widgetYToScene(event->pos().y());

    // Modulation drag
    if (m_modulationDragging && !m_selectedNotes.empty() && !m_preAdjustF0.empty()) {
        double dy = m_modulationDragStartY - sceneY;
        m_modulationDragAmount = std::clamp(1.0 + dy / ModulationDragSensitivity, 0.0, 5.0);
        applyModulationDriftPreview();
        return;
    }

    // Drift drag
    if (m_driftDragging && !m_selectedNotes.empty() && !m_preAdjustF0.empty()) {
        double dy = m_driftDragStartY - sceneY;
        m_driftDragAmount = std::clamp(1.0 + dy / ModulationDragSensitivity, 0.0, 5.0);
        applyModulationDriftPreview();
        return;
    }

    // Note pitch drag
    if (m_draggingNote && !m_selectedNotes.empty()) {
        double currentMidi = yToMidi(sceneY);
        double deltaMidi = currentMidi - m_dragStartMidi;
        int deltaCents = static_cast<int>(deltaMidi * 100);
        if (deltaCents != m_dragAccumulatedCents) {
            m_dragAccumulatedCents = deltaCents;
            update();
        }
        return;
    }

    // Panning (middle button only)
    if (m_isDragging && m_dragButton == Qt::MiddleButton) {
        QPoint delta = event->pos() - m_dragStart;
        double deltaSec = -delta.x() / m_hScale;
        m_viewport->scrollBy(deltaSec);
        m_vScrollBar->setValue(m_vScrollBar->value() - delta.y());
        m_dragStart = event->pos();
        return;
    }

    // Rubber-band selection
    if (m_rubberBandActive) {
        int x1 = std::min(m_rubberBandStart.x(), event->pos().x());
        int y1 = std::min(m_rubberBandStart.y(), event->pos().y());
        int x2 = std::max(m_rubberBandStart.x(), event->pos().x());
        int y2 = std::max(m_rubberBandStart.y(), event->pos().y());
        m_rubberBandRect = QRect(x1, y1, x2 - x1, y2 - y1);
        update();
        return;
    }

    // Repaint for crosshair
    if (m_showCrosshairAndPitch) {
        update();
    }
}

void PianoRollView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        m_isDragging = false;
        m_dragButton = Qt::NoButton;
        restoreToolCursor();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        // Finish ruler scrub
        if (m_rulerDragging) {
            m_rulerDragging = false;
            return;
        }

        // Finish note pitch drag
        if (m_draggingNote) {
            m_draggingNote = false;
            int delta = m_dragAccumulatedCents;
            if (delta != 0 && m_dsFile && !m_selectedNotes.empty()) {
                std::vector<int> indices(m_selectedNotes.begin(), m_selectedNotes.end());
                doPitchMove(indices, delta);
            }
            m_dragAccumulatedCents = 0;
            m_dragStartMidi = 0.0;
            m_dragOrigMidi.clear();
            restoreToolCursor();
            update();
            return;
        }

        // Finish modulation/drift drag
        if (m_modulationDragging || m_driftDragging) {
            // Push undo command with old (pre-drag) and new (current) F0 values
            if (m_undoStack && m_dsFile && !m_preAdjustF0.empty()) {
                m_undoStack->push(new ModulationDriftCommand(
                    m_dsFile, m_preAdjustF0, m_dsFile->f0.values));
            } else if (m_dsFile) {
                m_dsFile->markModified();
            }
            m_modulationDragging = false;
            m_driftDragging = false;
            m_preAdjustF0.clear();
            restoreToolCursor();
            emit fileEdited();
            return;
        }

        // Finish rubber-band selection
        if (m_rubberBandActive) {
            m_rubberBandActive = false;
            if (!m_rubberBandRect.isNull() && m_dsFile) {
                std::set<int> selected;
                for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i) {
                    const auto &note = m_dsFile->notes[i];
                    double noteMidi;
                    if (note.isRest()) {
                        noteMidi = getRestMidi(i);
                    } else {
                        auto pitch = parseNoteName(note.name);
                        if (!pitch.valid) continue;
                        noteMidi = pitch.midiNumber;
                    }
                    double sceneX1 = timeToX(note.start);
                    double sceneX2 = timeToX(note.end());
                    double sceneY = midiToY(noteMidi);
                    int wx1 = sceneXToWidget(sceneX1);
                    int wx2 = sceneXToWidget(sceneX2);
                    int wy = sceneYToWidget(sceneY);
                    QRect noteRect(wx1, wy, wx2 - wx1, static_cast<int>(m_vScale));
                    if (m_rubberBandRect.intersects(noteRect)) {
                        selected.insert(i);
                    }
                }
                selectNotes(selected);
            }
            m_rubberBandRect = QRect();
            restoreToolCursor();
            update();
            return;
        }

        m_isDragging = false;
        m_dragButton = Qt::NoButton;
        restoreToolCursor();
    }
}

void PianoRollView::contextMenuEvent(QContextMenuEvent *event) {
    // Check if right-click is on a note
    int noteIdx = getNoteAtPosition(event->pos().x(), event->pos().y());
    if (noteIdx >= 0 && m_noteMenu) {
        m_contextNoteIndex = noteIdx;
        updateNoteMenuState();
        m_noteMenu->exec(event->globalPos());
    } else if (m_bgMenu) {
        m_bgMenu->exec(event->globalPos());
    }
}

// ============================================================================
// Double-click: snap to semitone
// ============================================================================

void PianoRollView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();
        if (x < PianoWidth || y < RulerHeight) return;

        int clicked = getNoteAtPosition(x, y);
        if (clicked >= 0 && m_dsFile && !m_selectedNotes.empty()) {
            if (m_selectedNotes.count(clicked) == 0)
                selectNotes({clicked});
            // Snap all selected notes to nearest semitone (cents -> 0)
            for (int idx : m_selectedNotes) {
                if (idx < static_cast<int>(m_dsFile->notes.size())) {
                    auto &note = m_dsFile->notes[idx];
                    if (!note.isRest()) {
                        auto pitch = parseNoteName(note.name);
                        if (pitch.valid && pitch.cents != 0) {
                            note.name = shiftNoteCents(note.name, -pitch.cents);
                        }
                    }
                }
            }
            m_dsFile->markModified();
            emit fileEdited();
            update();
        }
    }
}

// ============================================================================
// Keyboard shortcuts
// ============================================================================

void PianoRollView::keyPressEvent(QKeyEvent *event) {
    int key = event->key();
    auto mods = event->modifiers();

    // PageUp/PageDown: let parent handle (file navigation)
    if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        event->ignore();
        return;
    }

    // Ctrl+A: select all
    if (key == Qt::Key_A && (mods & Qt::ControlModifier)) {
        selectAllNotes();
        event->accept();
        return;
    }

    // Escape: clear selection
    if (key == Qt::Key_Escape) {
        clearSelection();
        event->accept();
        return;
    }

    if (!m_dsFile || m_selectedNotes.empty()) {
        QFrame::keyPressEvent(event);
        return;
    }

    std::vector<int> indices(m_selectedNotes.begin(), m_selectedNotes.end());

    // Pitch move: ↑↓ 1¢, Shift 10¢, Ctrl 100¢
    if (key == Qt::Key_Up) {
        int delta = 1;
        if (mods & Qt::ControlModifier) delta = 100;
        else if (mods & Qt::ShiftModifier) delta = 10;
        doPitchMove(indices, delta);
        event->accept();
        return;
    }
    if (key == Qt::Key_Down) {
        int delta = -1;
        if (mods & Qt::ControlModifier) delta = -100;
        else if (mods & Qt::ShiftModifier) delta = -10;
        doPitchMove(indices, delta);
        event->accept();
        return;
    }

    // Delete
    if (key == Qt::Key_Delete) {
        emit noteDeleteRequested(indices);
        event->accept();
        return;
    }

    QFrame::keyPressEvent(event);
}

// ============================================================================
// Selection management
// ============================================================================

void PianoRollView::selectNotes(const std::set<int> &indices) {
    m_selectedNotes = indices;
    emit selectionChanged(m_selectedNotes);
    update();
}

void PianoRollView::selectAllNotes() {
    if (!m_dsFile) return;
    std::set<int> all;
    for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i)
        all.insert(i);
    selectNotes(all);
}

void PianoRollView::clearSelection() {
    selectNotes({});
}

// ============================================================================
// Pitch editing
// ============================================================================

void PianoRollView::doPitchMove(const std::vector<int> &indices, int deltaCents) {
    if (!m_dsFile || deltaCents == 0) return;
    if (m_undoStack) {
        m_undoStack->push(new PitchMoveCommand(m_dsFile, indices, deltaCents));
    } else {
        for (int idx : indices) {
            if (idx < 0 || idx >= static_cast<int>(m_dsFile->notes.size())) continue;
            auto &note = m_dsFile->notes[idx];
            if (note.isRest()) continue;
            note.name = shiftNoteCents(note.name, deltaCents);
        }
        m_dsFile->markModified();
    }
    emit fileEdited();
    update();
}

// ============================================================================
// Modulation/Drift preview (ported from Python AutoProcessor)
// ============================================================================

std::vector<double> PianoRollView::movingAverage(const std::vector<double> &values, int window) {
    std::vector<double> result(values.size(), 0.0);
    int half = window / 2;
    for (size_t i = 0; i < values.size(); ++i) {
        int lo = std::max(0, static_cast<int>(i) - half);
        int hi = std::min(static_cast<int>(values.size()), static_cast<int>(i) + half + 1);
        double sum = 0.0;
        int count = 0;
        for (int j = lo; j < hi; ++j) {
            if (values[j] > 0.0) { sum += values[j]; count++; }
        }
        result[i] = count > 0 ? sum / count : 0.0;
    }
    return result;
}

void PianoRollView::applyModulationDriftPreview() {
    if (!m_dsFile || m_preAdjustF0.empty()) return;

    auto &f0 = m_dsFile->f0;
    if (f0.timestep <= 0) return;

    // Restore from snapshot
    f0.values = m_preAdjustF0;

    double offset = m_dsFile->offset;

    for (int noteIdx : m_selectedNotes) {
        if (noteIdx < 0 || noteIdx >= static_cast<int>(m_dsFile->notes.size())) continue;
        const auto &note = m_dsFile->notes[noteIdx];
        if (note.isRest()) continue;
        auto pitch = parseNoteName(note.name);
        if (!pitch.valid) continue;

        int startIdx = std::max(0, static_cast<int>((note.start - offset) / f0.timestep));
        int endIdx = std::min(static_cast<int>(f0.values.size()),
                              static_cast<int>(std::ceil((note.end() - offset) / f0.timestep)));
        if (startIdx >= endIdx) continue;

        // Convert MIDI segment to Hz for processing (matching Python reference)
        int n = endIdx - startIdx;
        std::vector<double> segHz(n);
        std::vector<double> segMidi(n);
        bool hasVoiced = false;
        for (int i = 0; i < n; ++i) {
            double midi = f0.values[startIdx + i];
            segMidi[i] = midi;
            if (midi > 0.0) {
                segHz[i] = midiToFreq(midi);
                hasVoiced = true;
            } else {
                segHz[i] = 0.0;
            }
        }
        if (!hasVoiced) continue;

        double targetFreq = midiToFreq(pitch.midiNumber);
        if (targetFreq <= 0) continue;

        int window = std::max(5, n / 4);
        if (window % 2 == 0) window++;

        auto centerline = movingAverage(segHz, window);

        std::vector<double> newVals = segHz;
        for (int i = 0; i < n; ++i) {
            if (segHz[i] <= 0) continue;
            double modulationI = segHz[i] - centerline[i];
            double driftDev = centerline[i] - targetFreq;
            double newCenter = targetFreq + driftDev * m_driftDragAmount;
            double newMod = modulationI * m_modulationDragAmount;
            newVals[i] = std::clamp(newCenter + newMod,
                                    midiToFreq(24.0), midiToFreq(108.0));
        }

        // Smoothstep crossfade at edges
        int crossfade = std::clamp(n / 8, 3, n / 2);
        for (int i = 0; i < crossfade; ++i) {
            if (segHz[i] <= 0) continue;
            double t = static_cast<double>(i + 1) / (crossfade + 1);
            t = t * t * (3 - 2 * t);
            newVals[i] = segHz[i] * (1 - t) + newVals[i] * t;
        }
        for (int i = 0; i < crossfade; ++i) {
            int ri = n - 1 - i;
            if (segHz[ri] <= 0) continue;
            double t = static_cast<double>(i + 1) / (crossfade + 1);
            t = t * t * (3 - 2 * t);
            newVals[ri] = segHz[ri] * (1 - t) + newVals[ri] * t;
        }

        // Convert back to MIDI and write
        for (int i = 0; i < n; ++i) {
            if (startIdx + i < static_cast<int>(f0.values.size())) {
                if (newVals[i] > 0.0)
                    f0.values[startIdx + i] = freqToMidi(newVals[i]);
                else
                    f0.values[startIdx + i] = 0.0;
            }
        }
    }

    update();
}

// ============================================================================
// Drawing: rubber band + snap guide
// ============================================================================

void PianoRollView::drawRubberBand(QPainter &p) {
    if (!m_rubberBandActive || m_rubberBandRect.isNull()) return;
    p.save();
    p.setPen(QPen(Colors::RubberBandBorder, 1, Qt::DashLine));
    p.setBrush(Colors::RubberBandFill);
    p.drawRect(m_rubberBandRect);
    p.restore();
}

void PianoRollView::drawSnapGuide(QPainter &p, int w, int h) {
    if (!m_draggingNote || m_selectedNotes.empty() || !m_dsFile || m_dragAccumulatedCents == 0)
        return;

    p.save();
    p.setClipRect(PianoWidth, RulerHeight, w - PianoWidth, h - RulerHeight);

    int firstIdx = *m_selectedNotes.begin();
    if (firstIdx >= static_cast<int>(m_dsFile->notes.size())) { p.restore(); return; }

    auto it = m_dragOrigMidi.find(firstIdx);
    if (it == m_dragOrigMidi.end()) { p.restore(); return; }

    double origMidi = it->second;
    double newMidi = origMidi + m_dragAccumulatedCents / 100.0;
    const auto &note = m_dsFile->notes[firstIdx];

    double x1 = timeToX(note.start);
    double x2 = timeToX(note.end());
    int wx1 = sceneXToWidget(x1);
    int wx2 = sceneXToWidget(x2);
    int origWy = sceneYToWidget(midiToY(origMidi));
    int newWy = sceneYToWidget(midiToY(newMidi));
    int midX = (wx1 + wx2) / 2;

    QPen guidePen(Colors::SnapGuide, 1.5, Qt::DashLine);
    p.setPen(guidePen);
    // Horizontal line at original position
    p.drawLine(wx1, origWy, wx2, origWy);
    // Vertical line connecting original to new
    QPen vertPen(Colors::SnapGuide, 1.0, Qt::DashLine);
    p.setPen(vertPen);
    p.drawLine(midX, origWy, midX, newWy);
    // Horizontal line at new position
    p.drawLine(wx1, newWy, wx2, newWy);

    // Note name label
    QString label = midiToNoteString(origMidi);
    QFont font("Segoe UI", 9, QFont::Bold);
    p.setFont(font);
    p.setPen(Colors::SnapGuide);
    QFontMetrics fm(font);
    p.drawText(wx1 - fm.horizontalAdvance(label) - 4,
               origWy + fm.ascent() / 2, label);

    p.restore();
}

// ============================================================================
// Config persistence (following SlurCutter F0Widget pattern)
// ============================================================================

void PianoRollView::loadConfig(dstools::AppSettings &settings) {
    m_snapToKey = settings.get(PitchLabelerKeys::SnapToKey);
    m_showPitchTextOverlay = settings.get(PitchLabelerKeys::ShowPitchTextOverlay);
    m_showPhonemeTexts = settings.get(PitchLabelerKeys::ShowPhonemeTexts);
    m_showCrosshairAndPitch = settings.get(PitchLabelerKeys::ShowCrosshairAndPitch);

    if (m_actSnapToKey) m_actSnapToKey->setChecked(m_snapToKey);
    if (m_actShowPitchTextOverlay) m_actShowPitchTextOverlay->setChecked(m_showPitchTextOverlay);
    if (m_actShowPhonemeTexts) m_actShowPhonemeTexts->setChecked(m_showPhonemeTexts);
    if (m_actShowCrosshairAndPitch) m_actShowCrosshairAndPitch->setChecked(m_showCrosshairAndPitch);

    update();
}

void PianoRollView::pullConfig(dstools::AppSettings &settings) const {
    settings.set(PitchLabelerKeys::SnapToKey, m_snapToKey);
    settings.set(PitchLabelerKeys::ShowPitchTextOverlay, m_showPitchTextOverlay);
    settings.set(PitchLabelerKeys::ShowPhonemeTexts, m_showPhonemeTexts);
    settings.set(PitchLabelerKeys::ShowCrosshairAndPitch, m_showCrosshairAndPitch);
}

// ============================================================================
// ViewportController integration
// ============================================================================

void PianoRollView::setViewportController(dstools::widgets::ViewportController *vc) {
    Q_ASSERT(vc);
    m_viewport = vc;
    if (vc) {
        connect(vc, &dstools::widgets::ViewportController::viewportChanged,
                this, &PianoRollView::onViewportChanged);
        // Initialize viewport from current state
        vc->setPixelsPerSecond(m_hScale);
    }
}

void PianoRollView::onViewportChanged(const dstools::widgets::ViewportState &state) {
    m_hScale = state.pixelsPerSecond;
    m_scrollX = state.startSec * state.pixelsPerSecond;
    updateScrollBars();
    // Sync scrollbar position without triggering feedback
    m_hScrollBar->blockSignals(true);
    m_hScrollBar->setValue(static_cast<int>(m_scrollX));
    m_hScrollBar->blockSignals(false);
    update();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
