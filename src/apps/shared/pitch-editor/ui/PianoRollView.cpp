#include "PianoRollView.h"
#include "DSFile.h"
#include "commands/PitchCommands.h"

#include "PitchLabelerKeys.h"

#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QDebug>

#include <dsfw/AppSettings.h>

#include <cmath>
#include <algorithm>

namespace dstools {
namespace pitchlabeler {
namespace ui {

// ============================================================================
// Construction / Setup
// ============================================================================

PianoRollView::PianoRollView(QWidget *parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    setMinimumSize(400, 300);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_hScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_vScrollBar = new QScrollBar(Qt::Vertical, this);

    connect(m_hScrollBar, &QScrollBar::valueChanged, this, [this](int val) {
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
    setupInputCallbacks();

    // Center view on middle C area by default
    m_scrollY = static_cast<int>((MaxMidi - 72) * m_vScale);
}

PianoRollView::~PianoRollView() = default;

void PianoRollView::setupInputCallbacks() {
    InputHandlerCallbacks cb;
    cb.widgetXToScene = [this](int wx) { return widgetXToScene(wx); };
    cb.widgetYToScene = [this](int wy) { return widgetYToScene(wy); };
    cb.xToTime = [this](double x) { return xToTime(x); };
    cb.yToMidi = [this](double y) { return yToMidi(y); };
    cb.timeToX = [this](double t) { return timeToX(t); };
    cb.midiToY = [this](double m) { return midiToY(m); };
    cb.sceneXToWidget = [this](double sx) { return sceneXToWidget(sx); };
    cb.sceneYToWidget = [this](double sy) { return sceneYToWidget(sy); };
    cb.update = [this]() { update(); };
    cb.setCursor = [this](Qt::CursorShape c) { setCursor(c); };
    cb.selectNotes = [this](const std::set<int> &s) { selectNotes(s); };
    cb.selectAllNotes = [this]() { selectAllNotes(); };
    cb.clearSelection = [this]() { clearSelection(); };
    cb.doPitchMove = [this](const std::vector<int> &i, int d) { doPitchMove(i, d); };
    cb.restoreToolCursor = [this]() { restoreToolCursor(); };
    cb.getNoteAtPosition = [this](int x, int y) { return getNoteAtPosition(x, y); };
    cb.getRestMidi = [this](int i) {
        return m_dsFile ? PitchProcessor::getRestMidi(*m_dsFile, i) : 60.0;
    };
    cb.viewportZoomAt = [this](double center, double factor) {
        m_viewport->zoomAt(center, factor);
    };
    cb.viewportScrollBy = [this](double d) { m_viewport->scrollBy(d); };
    cb.viewportViewCenter = [this]() { return m_viewport->viewCenter(); };
    cb.viewportSetPPS = [this](double pps) { m_viewport->setPixelsPerSecond(pps); };
    cb.setVScrollValue = [this](int v) { m_vScrollBar->setValue(v); };
    cb.getVScrollValue = [this]() { return m_vScrollBar->value(); };
    cb.emitNoteSelected = [this](int i) { emit noteSelected(i); };
    cb.emitPositionClicked = [this](double t, double m) { emit positionClicked(t, m); };
    cb.emitRulerClicked = [this](double t) {
        m_playheadPos = t;
        emit rulerClicked(t);
    };
    cb.emitFileEdited = [this]() { emit fileEdited(); };
    cb.emitNoteDeleteRequested = [this](const std::vector<int> &i) {
        emit noteDeleteRequested(i);
    };
    m_inputHandler.setCallbacks(cb);
}

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

// ============================================================================
// Context Menus (unchanged)
// ============================================================================

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

    m_actMergeLeft->setEnabled(note.isSlur() && m_contextNoteIndex > 0);

    m_actToggleRest->setText(note.isRest()
        ? QStringLiteral("转为普通音符(&R)")
        : QStringLiteral("标记为休止(&R)"));

    m_actToggleSlur->setText(note.isSlur()
        ? QStringLiteral("取消连音(&L)")
        : QStringLiteral("设为连音(&L)"));

    if (note.glide == "up") m_actGlideUp->setChecked(true);
    else if (note.glide == "down") m_actGlideDown->setChecked(true);
    else m_actGlideNone->setChecked(true);
}

// ============================================================================
// Data management
// ============================================================================

void PianoRollView::setDSFile(std::shared_ptr<pitchlabeler::DSFile> ds) {
    m_dsFile = ds;
    m_selectedNotes.clear();
    m_inputHandler.reset();

    if (ds && !ds->f0.values.empty()) {
        m_originalF0 = ds->f0.values;
    } else {
        m_originalF0.clear();
    }

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
    m_inputHandler.reset();
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

void PianoRollView::storeOriginalF0(const std::vector<int32_t> &original) {
    m_originalF0 = original;
}

void PianoRollView::setToolMode(ToolMode mode) {
    if (m_toolMode != mode) {
        m_toolMode = mode;
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

    double totalDuration = m_audioDuration > 0 ? m_audioDuration
                         : m_dsFile            ? usToSec(m_dsFile->getTotalDuration())
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

int PianoRollView::getNoteAtPosition(int x, int y) const {
    if (!m_dsFile) return -1;

    double sceneX = widgetXToScene(x);
    double sceneY = widgetYToScene(y);
    double time = xToTime(sceneX);
    double midi = yToMidi(sceneY);

    for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i) {
        const auto &note = m_dsFile->notes[i];
        double noteStartSec = usToSec(note.start);
        double noteEndSec = usToSec(note.end());
        if (time >= noteStartSec && time < noteEndSec) {
            if (note.isRest()) {
                double restMidi = PitchProcessor::getRestMidi(*m_dsFile, i);
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
// Render state builder
// ============================================================================

RenderState PianoRollView::buildRenderState() const {
    RenderState s;
    s.dsFile = m_dsFile.get();
    s.hScale = m_hScale;
    s.vScale = m_vScale;
    s.scrollX = m_scrollX;
    s.scrollY = m_scrollY;
    s.audioDuration = m_audioDuration;
    s.playheadPos = m_playheadPos;
    s.isPlaying = m_isPlaying;
    s.selectedNotes = &m_selectedNotes;
    s.showPitchTextOverlay = m_showPitchTextOverlay;
    s.showPhonemeTexts = m_showPhonemeTexts;
    s.showCrosshairAndPitch = m_showCrosshairAndPitch;
    s.mousePos = m_inputHandler.mousePos();
    s.draggingNote = m_inputHandler.isDraggingNote();
    s.dragAccumulatedCents = m_inputHandler.dragAccumulatedCents();
    s.dragOrigMidi = &m_inputHandler.dragOrigMidi();
    s.rubberBandActive = m_inputHandler.isRubberBandActive();
    s.rubberBandRect = m_inputHandler.rubberBandRect();
    s.abComparisonActive = m_abComparisonActive;
    s.originalF0 = &m_originalF0;
    return s;
}

// ============================================================================
// Paint — delegates to PianoRollRenderer
// ============================================================================

void PianoRollView::paintEvent(QPaintEvent * /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width() - ScrollBarSize;
    int h = height() - ScrollBarSize;

    auto rs = buildRenderState();

    p.fillRect(0, 0, w, h, Colors::Background);
    p.setClipRect(0, 0, w, h);

    PianoRollRenderer::drawGrid(p, w, h, rs);
    PianoRollRenderer::drawNotes(p, w, h, rs);
    PianoRollRenderer::drawF0Curve(p, w, h, rs);
    PianoRollRenderer::drawPlayhead(p, w, h, rs);
    PianoRollRenderer::drawCrosshair(p, w, h, rs);
    PianoRollRenderer::drawSnapGuide(p, w, h, rs);
    PianoRollRenderer::drawRubberBand(p, rs);
    PianoRollRenderer::drawRuler(p, w, rs);
    PianoRollRenderer::drawPianoKeys(p, h, rs);

    // Corner patch
    p.setClipRect(0, 0, w, h);
    p.fillRect(0, 0, PianoWidth, RulerHeight, Colors::RulerBg);

    // Position scroll bars
    m_hScrollBar->setGeometry(0, h, w, ScrollBarSize);
    m_vScrollBar->setGeometry(w, 0, ScrollBarSize, h);
}

// ============================================================================
// Events — delegate to InputHandler
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
    m_inputHandler.handleWheel(event);
}

void PianoRollView::mousePressEvent(QMouseEvent *event) {
    m_inputHandler.handleMousePress(event, m_toolMode, m_dsFile, m_selectedNotes);
}

void PianoRollView::mouseMoveEvent(QMouseEvent *event) {
    m_inputHandler.handleMouseMove(event, m_toolMode, m_dsFile, m_selectedNotes,
                                   m_showCrosshairAndPitch);
}

void PianoRollView::mouseReleaseEvent(QMouseEvent *event) {
    m_inputHandler.handleMouseRelease(event, m_toolMode, m_dsFile, m_selectedNotes,
                                      m_undoStack);
}

void PianoRollView::mouseDoubleClickEvent(QMouseEvent *event) {
    m_inputHandler.handleMouseDoubleClick(event, m_dsFile, m_selectedNotes, m_undoStack);
}

void PianoRollView::keyPressEvent(QKeyEvent *event) {
    m_inputHandler.handleKeyPress(event, m_dsFile, m_selectedNotes);
    if (!event->isAccepted()) {
        QFrame::keyPressEvent(event);
    }
}

void PianoRollView::contextMenuEvent(QContextMenuEvent *event) {
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
// Pitch editing — delegates math to PitchProcessor
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
// Config persistence
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
        vc->setPixelsPerSecond(m_hScale);
    }
}

void PianoRollView::onViewportChanged(const dstools::widgets::ViewportState &state) {
    m_hScale = (state.sampleRate > 0 && state.resolution > 0)
        ? static_cast<double>(state.sampleRate) / state.resolution
        : 200.0;
    m_scrollX = state.startSec * m_hScale;
    updateScrollBars();
    m_hScrollBar->blockSignals(true);
    m_hScrollBar->setValue(static_cast<int>(m_scrollX));
    m_hScrollBar->blockSignals(false);
    update();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
