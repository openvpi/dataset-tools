#include "PianoRollView.h"
#include <dstools/DsPitchDocument.h>
#include "commands/PitchCommands.h"
#include "commands/SplitMergeCommands.h"

#include "Keys.h"

#include <QContextMenuEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>

#include <dsfw/AppSettings.h>

#include <algorithm>
#include <cmath>

namespace dstools {
    namespace pitchlabeler {
        namespace ui {

            // ============================================================================
            // Construction / Setup
            // ============================================================================

            PianoRollView::PianoRollView(QWidget *parent) : QFrame(parent) {
                setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
                setMinimumSize(400, 300);
                setMouseTracking(true);
                setFocusPolicy(Qt::StrongFocus);

                m_vScrollBar = new QScrollBar(Qt::Vertical, this);

                connect(m_vScrollBar, &QScrollBar::valueChanged, this, [this](int val) {
                    m_scrollY = val;
                    m_coord.scrollY = val;
                    emit verticalScrolled();
                    update();
                });

                buildContextMenu();
                buildNoteMenu();
                setupInputCallbacks();

                // Center view on middle C area by default
                m_scrollY = static_cast<int>((RenderState::MaxMidi - 72) * m_vScale);
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
                cb.getNoteBoundaryAtPosition = [this](int x, int y) { return getNoteBoundaryAtPosition(x, y); };
                cb.getRestMidi = [this](int i) { return m_dsFile ? PitchProcessor::getRestMidi(*m_dsFile, i) : 60.0; };
                 cb.viewportScrollBy = [this](double d) {
                    if (!m_viewport) return;
                    m_viewport->scrollBy(d);
                };
                cb.setVScrollValue = [this](int v) { m_vScrollBar->setValue(v); };
                cb.getVScrollValue = [this]() { return m_vScrollBar->value(); };
                cb.emitNoteSelected = [this](int i) { emit noteSelected(i); };
                cb.emitPositionClicked = [this](double t, double m) { emit positionClicked(t, m); };
                cb.unusedCallback1 = [this](double) { };
                cb.emitFileEdited = [this]() { emit fileEdited(); };
                cb.emitNoteDeleteRequested = [this](const std::vector<int> &i) { emit noteDeleteRequested(i); };
                cb.startBoundaryDrag = [this](int boundaryIndex) { startBoundaryDrag(boundaryIndex); };
                cb.updateBoundaryDrag = [this](int boundaryIndex, int deltaPixels) { updateBoundaryDrag(boundaryIndex, deltaPixels); };
                cb.endBoundaryDrag = [this]() { endBoundaryDrag(); };
                cb.cancelBoundaryDrag = [this]() { cancelBoundaryDrag(); };
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

                auto *zoomIn = new QAction(tr("Zoom In"), m_bgMenu);
                zoomIn->setShortcut(QKeySequence("Ctrl+="));
                connect(zoomIn, &QAction::triggered, this, &PianoRollView::zoomIn);
                m_bgMenu->addAction(zoomIn);

                auto *zoomOut = new QAction(tr("Zoom Out"), m_bgMenu);
                zoomOut->setShortcut(QKeySequence("Ctrl+-"));
                connect(zoomOut, &QAction::triggered, this, &PianoRollView::zoomOut);
                m_bgMenu->addAction(zoomOut);

                m_bgMenu->addSeparator();

                auto *reset = new QAction(tr("Reset View"), m_bgMenu);
                reset->setShortcut(QKeySequence("Ctrl+0"));
                connect(reset, &QAction::triggered, this, &PianoRollView::resetZoom);
                m_bgMenu->addAction(reset);

                m_bgMenu->addSeparator();

                auto *optionsPrompt = new QAction(tr("Display Options"), m_bgMenu);
                auto boldFont = optionsPrompt->font();
                boldFont.setBold(true);
                optionsPrompt->setFont(boldFont);
                optionsPrompt->setEnabled(false);
                m_bgMenu->addAction(optionsPrompt);

                m_actSnapToKey = new QAction(tr("Snap to Keys(&S)"), m_bgMenu);
                m_actSnapToKey->setCheckable(true);
                m_actSnapToKey->setChecked(m_snapToKey);
                connect(m_actSnapToKey, &QAction::triggered, this, [this] {
                    m_snapToKey = m_actSnapToKey->isChecked();
                    update();
                });
                m_bgMenu->addAction(m_actSnapToKey);

                m_actShowPitchTextOverlay = new QAction(tr("Show Pitch Overlay(&O)"), m_bgMenu);
                m_actShowPitchTextOverlay->setCheckable(true);
                m_actShowPitchTextOverlay->setChecked(m_showPitchTextOverlay);
                connect(m_actShowPitchTextOverlay, &QAction::triggered, this, [this] {
                    m_showPitchTextOverlay = m_actShowPitchTextOverlay->isChecked();
                    update();
                });
                m_bgMenu->addAction(m_actShowPitchTextOverlay);

                m_actShowPhonemeTexts = new QAction(tr("Show Phonemes(&P)"), m_bgMenu);
                m_actShowPhonemeTexts->setCheckable(true);
                m_actShowPhonemeTexts->setChecked(m_showPhonemeTexts);
                connect(m_actShowPhonemeTexts, &QAction::triggered, this, [this] {
                    m_showPhonemeTexts = m_actShowPhonemeTexts->isChecked();
                    update();
                });
                m_bgMenu->addAction(m_actShowPhonemeTexts);

                m_actShowCrosshairAndPitch = new QAction(tr("Show Crosshair and Pitch(&C)"), m_bgMenu);
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

                m_actMergeLeft = new QAction(tr("Merge Left(&M)"), m_noteMenu);
                connect(m_actMergeLeft, &QAction::triggered, this, [this] {
                    if (m_contextNoteIndex >= 0)
                        emit noteMergeLeft(m_contextNoteIndex);
                });
                m_noteMenu->addAction(m_actMergeLeft);

                m_actToggleRest = new QAction(tr("Toggle Rest(&R)"), m_noteMenu);
                connect(m_actToggleRest, &QAction::triggered, this, [this] {
                    if (m_contextNoteIndex >= 0)
                        emit noteRestToggled(m_contextNoteIndex);
                });
                m_noteMenu->addAction(m_actToggleRest);

                m_actToggleSlur = new QAction(tr("Toggle Slur(&L)"), m_noteMenu);
                connect(m_actToggleSlur, &QAction::triggered, this, [this] {
                    if (m_contextNoteIndex >= 0)
                        emit noteSlurToggled(m_contextNoteIndex);
                });
                m_noteMenu->addAction(m_actToggleSlur);

                m_noteMenu->addSeparator();

                auto *glidePrompt = new QAction(tr("Ornament: Glide"), m_noteMenu);
                glidePrompt->setFont(boldFont);
                glidePrompt->setEnabled(false);
                m_noteMenu->addAction(glidePrompt);

                m_actGlideGroup = new QActionGroup(this);
                m_actGlideNone = new QAction(tr("None"), m_noteMenu);
                m_actGlideUp = new QAction(tr("Glide Up"), m_noteMenu);
                m_actGlideDown = new QAction(tr("Glide Down"), m_noteMenu);
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
                    if (m_contextNoteIndex < 0)
                        return;
                    QString glide = "none";
                    if (action == m_actGlideUp)
                        glide = "up";
                    else if (action == m_actGlideDown)
                        glide = "down";
                    emit noteGlideChanged(m_contextNoteIndex, glide);
                });
            }

            void PianoRollView::updateNoteMenuState() {
                if (!m_dsFile || m_contextNoteIndex < 0 ||
                    m_contextNoteIndex >= static_cast<int>(m_dsFile->notes.size()))
                    return;

                const auto &note = m_dsFile->notes[m_contextNoteIndex];

                m_actMergeLeft->setEnabled(note.isSlur() && m_contextNoteIndex > 0);

                m_actToggleRest->setText(note.isRest() ? tr("Convert to Note(&R)")
                                                       : tr("Mark as Rest(&R)"));

                m_actToggleSlur->setText(note.isSlur() ? tr("Cancel Slur(&L)")
                                                       : tr("Set as Slur(&L)"));

                if (note.glide == "up")
                    m_actGlideUp->setChecked(true);
                else if (note.glide == "down")
                    m_actGlideDown->setChecked(true);
                else
                    m_actGlideNone->setChecked(true);
            }

            // ============================================================================
            // Data management
            // ============================================================================

            void PianoRollView::setDsPitchDocument(std::shared_ptr<pitchlabeler::DsPitchDocument> ds) {
                m_dsFile = ds;
                m_selectedNotes.clear();
                m_inputHandler.reset();

                // Create boundary model for the DsPitchDocument
                if (ds) {
                    m_boundaryModel = std::make_unique<NoteBoundaryModel>();
                    m_boundaryModel->setDsPitchDocument(ds);
                } else {
                    m_boundaryModel.reset();
                }

                if (ds && !ds->f0.values.empty()) {
                    m_originalF0 = ds->f0.values;
                } else {
                    m_originalF0.clear();
                }

                if (ds && !ds->notes.empty()) {
                    double minMidi = 127, maxMidi = 0;
                    for (const auto &note : ds->notes) {
                        if (note.isRest())
                            continue;
                        auto pitch = parseNoteName(note.name);
                        if (pitch.valid) {
                            minMidi = std::min(minMidi, pitch.midiNumber);
                            maxMidi = std::max(maxMidi, pitch.midiNumber);
                        }
                    }
                    if (maxMidi > minMidi) {
                        double centerMidi = (minMidi + maxMidi) / 2.0;
                        int drawH = height() - RenderState::RulerHeight;
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
                updateScrollBars();
                update();
            }

             void PianoRollView::zoomIn() {
        if (!m_viewport) return;
        double centerSec = m_viewport->viewCenter();
        m_viewport->zoomIn(centerSec);
        updateScrollBars();
        update();
    }

     void PianoRollView::zoomOut() {
        if (!m_viewport) return;
        double centerSec = m_viewport->viewCenter();
        m_viewport->zoomOut(centerSec);
        updateScrollBars();
        update();
    }

     void PianoRollView::resetZoom() {
        if (!m_viewport) return;

        m_viewport->setResolution(m_defaultResolution);

        if (m_audioDuration > 0) {
            m_viewport->setViewRange(0.0, m_audioDuration);
        } else {
            m_viewport->setViewRange(0.0, 10.0);
        }

        m_vScale = 20.0;

        double centerMidi = 72.0;
        if (m_dsFile && !m_dsFile->notes.empty()) {
            double minMidi = 127.0, maxMidi = 0.0;
            for (const auto &note : m_dsFile->notes) {
                if (note.isRest())
                    continue;
                auto pitch = parseNoteName(note.name);
                if (pitch.valid) {
                    minMidi = std::min(minMidi, pitch.midiNumber);
                    maxMidi = std::max(maxMidi, pitch.midiNumber);
                }
            }
            if (maxMidi > minMidi) {
                centerMidi = (minMidi + maxMidi) / 2.0;
            }
        }
        int drawH = height() - RenderState::RulerHeight;
        m_scrollY = std::max(0.0, midiToY(centerMidi) - drawH / 2.0);

        updateScrollBars();
        update();
    }

             int PianoRollView::getZoomPercent() const {
                if (m_coord.pixelWidth <= 0 || m_coord.viewEnd <= m_coord.viewStart)
                    return 100;
                double pps = m_coord.pixelWidth / (m_coord.viewEnd - m_coord.viewStart);
                return static_cast<int>(pps / 100.0 * 100.0);
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
                return m_coord.timeToX(time);
            }

            double PianoRollView::xToTime(double x) const {
                return m_coord.xToTime(static_cast<int>(std::round(x)));
            }

            double PianoRollView::midiToY(double midi) const {
                return (RenderState::MaxMidi - midi) * m_vScale + RenderState::RulerHeight;
            }

            double PianoRollView::yToMidi(double y) const {
                return RenderState::MaxMidi - (y - RenderState::RulerHeight) / m_vScale;
            }

            int PianoRollView::sceneXToWidget(double sceneX) const {
                return static_cast<int>(sceneX);
            }

            int PianoRollView::sceneYToWidget(double sceneY) const {
                return m_coord.sceneYToWidget(sceneY);
            }

            double PianoRollView::widgetXToScene(int wx) const {
                return static_cast<double>(wx);
            }

            double PianoRollView::widgetYToScene(int wy) const {
                return m_coord.widgetYToScene(wy);
            }

            // ============================================================================
            // Scroll bars
            // ============================================================================

             void PianoRollView::updateScrollBars() {
                int drawH = height();
                double sceneH = midiToY(RenderState::MinMidi) + 50;

                m_vScrollBar->setRange(0, qMax(0, static_cast<int>(sceneH - drawH)));
                m_vScrollBar->setPageStep(drawH);
            }

            // ============================================================================
            // Note hit testing
            // ============================================================================

            int PianoRollView::getNoteAtPosition(int x, int y) const {
                if (!m_dsFile)
                    return -1;

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
                            if (std::abs(midi - restMidi) < 1.0)
                                return i;
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

            int PianoRollView::getNoteBoundaryAtPosition(int x, int y) const {
                if (!m_dsFile || !m_boundaryModel)
                    return -1;

                double sceneX = widgetXToScene(x);
                double sceneY = widgetYToScene(y);
                double time = xToTime(sceneX);
                double midi = yToMidi(sceneY);

                // Check if we're near a note boundary
                const double sceneRadius = xToTime(m_boundaryHitRadius) - xToTime(0);

                for (int i = 0; i < static_cast<int>(m_dsFile->notes.size()); ++i) {
                    const auto &note = m_dsFile->notes[i];
                    double noteStartSec = usToSec(note.start);
                    double noteEndSec = usToSec(note.end());

                    // Check if we're near the start boundary
                    if (std::abs(time - noteStartSec) < sceneRadius) {
                        // Check if we're at the right vertical position
                        if (note.isRest()) {
                            double restMidi = PitchProcessor::getRestMidi(*m_dsFile, i);
                            if (std::abs(midi - restMidi) < 1.0)
                                return i * 2; // Start boundary index
                        } else {
                            auto pitch = parseNoteName(note.name);
                            if (pitch.valid && std::abs(midi - pitch.midiNumber) < 1.0) {
                                return i * 2; // Start boundary index
                            }
                        }
                    }

                    // Check if we're near the end boundary
                    if (std::abs(time - noteEndSec) < sceneRadius) {
                        // Check if we're at the right vertical position
                        if (note.isRest()) {
                            double restMidi = PitchProcessor::getRestMidi(*m_dsFile, i);
                            if (std::abs(midi - restMidi) < 1.0)
                                return i * 2 + 1; // End boundary index
                        } else {
                            auto pitch = parseNoteName(note.name);
                            if (pitch.valid && std::abs(midi - pitch.midiNumber) < 1.0) {
                                return i * 2 + 1; // End boundary index
                            }
                        }
                    }
                }
                return -1;
            }

            // ============================================================================
            // Boundary dragging
            // ============================================================================

            void PianoRollView::startBoundaryDrag(int boundaryIndex) {
                if (!m_dsFile || !m_boundaryModel || boundaryIndex < 0)
                    return;

                m_isDraggingBoundary = true;
                m_draggingBoundaryIndex = boundaryIndex;
                setCursor(Qt::SizeHorCursor);
            }

            void PianoRollView::updateBoundaryDrag(int boundaryIndex, int deltaPixels) {
                if (!m_dsFile || !m_boundaryModel || !m_isDraggingBoundary || boundaryIndex != m_draggingBoundaryIndex)
                    return;

                // Convert pixel delta to time delta
                double deltaTime = xToTime(deltaPixels) - xToTime(0);
                int64_t deltaUs = secToUs(deltaTime);

                // Update boundary model
                // Get current boundary time and add delta
                int64_t currentTime = m_boundaryModel->boundaryTime(0, boundaryIndex);
                int64_t newTime = currentTime + deltaUs;
                m_boundaryModel->moveBoundary(0, boundaryIndex, newTime);

                // Update view
                update();
                emit fileEdited();
            }

            void PianoRollView::endBoundaryDrag() {
                if (!m_isDraggingBoundary)
                    return;

                m_isDraggingBoundary = false;
                m_draggingBoundaryIndex = -1;
                restoreToolCursor();
            }

            void PianoRollView::cancelBoundaryDrag() {
                if (!m_isDraggingBoundary)
                    return;

                m_isDraggingBoundary = false;
                m_draggingBoundaryIndex = -1;
                restoreToolCursor();
            }

            // ============================================================================
            // Render state builder
            // ============================================================================

              RenderState PianoRollView::buildRenderState() const {
                RenderState s;
                s.dsFile = m_dsFile.get();
                s.vScale = m_vScale;
                s.scrollX = 0.0;
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

                s.coord = m_coord;

                s.contentLeft = m_contentLeftMargin;

                return s;
            }

            // ============================================================================
            // Paint — delegates to PianoRollRenderer
 // ============================================================================
            // Rendering

            void PianoRollView::render(QPainter &painter, const chart::ChartCoordinate &coord) {
                auto rs = buildRenderState();
                rs.coord = coord;
                rs.coord.scrollX = m_coord.scrollX;
                rs.coord.scrollY = m_coord.scrollY;
                int w = width() - RenderState::ScrollBarSize;
                int h = height();
                PianoRollRenderer::drawGrid(painter, w, h, rs);
                PianoRollRenderer::drawNotes(painter, w, h, rs);
                PianoRollRenderer::drawF0Curve(painter, w, h, rs);
                PianoRollRenderer::drawPlayhead(painter, w, h, rs);
                PianoRollRenderer::drawCrosshair(painter, w, h, rs);
                PianoRollRenderer::drawSnapGuide(painter, w, h, rs);
                PianoRollRenderer::drawRubberBand(painter, rs);
            }

            // ============================================================================

            void PianoRollView::paintEvent(QPaintEvent * /*event*/) {
                QPainter p(this);
                p.setRenderHint(QPainter::Antialiasing);

                int w = width() - RenderState::ScrollBarSize;
                int h = height();

                auto rs = buildRenderState();

                p.setClipRect(0, 0, w, h);
                p.fillRect(m_contentLeftMargin, 0, w - m_contentLeftMargin, h, Colors::Background);

                PianoRollRenderer::drawGrid(p, w, h, rs);
                PianoRollRenderer::drawNotes(p, w, h, rs);
                PianoRollRenderer::drawF0Curve(p, w, h, rs);
                PianoRollRenderer::drawPlayhead(p, w, h, rs);
                PianoRollRenderer::drawCrosshair(p, w, h, rs);
                PianoRollRenderer::drawSnapGuide(p, w, h, rs);
                PianoRollRenderer::drawRubberBand(p, rs);

                p.setClipRect(0, 0, w, h);

                // Position vertical scroll bar on the right
                m_vScrollBar->setGeometry(w, 0, RenderState::ScrollBarSize, h);
            }

            // ============================================================================
            // Events — delegate to InputHandler
            // ============================================================================

             void PianoRollView::resizeEvent(QResizeEvent *event) {
                QFrame::resizeEvent(event);
                updateCoord();
                updateScrollBars();
            }

            void PianoRollView::wheelEvent(QWheelEvent *event) {
                // Ctrl+wheel and Shift+wheel are handled by AudioVisualizerContainer
                if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) {
                    event->ignore();
                    return;
                }
                m_inputHandler.handleWheel(event);
            }

            void PianoRollView::mousePressEvent(QMouseEvent *event) {
                m_inputHandler.handleMousePress(event, m_toolMode, m_dsFile, m_selectedNotes);
            }

            void PianoRollView::mouseMoveEvent(QMouseEvent *event) {
                m_inputHandler.handleMouseMove(event, m_toolMode, m_dsFile, m_selectedNotes, m_showCrosshairAndPitch);
            }

            void PianoRollView::mouseReleaseEvent(QMouseEvent *event) {
                m_inputHandler.handleMouseRelease(event, m_toolMode, m_dsFile, m_selectedNotes, m_undoStack);
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
                if (!m_dsFile)
                    return;
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
                if (!m_dsFile || deltaCents == 0)
                    return;
                if (m_undoStack) {
                    m_undoStack->push(new PitchMoveCommand(m_dsFile, indices, deltaCents));
                } else {
                    for (int idx : indices) {
                        if (idx < 0 || idx >= static_cast<int>(m_dsFile->notes.size()))
                            continue;
                        auto &note = m_dsFile->notes[idx];
                        if (note.isRest())
                            continue;
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
                m_snapToKey = settings.get(dstools::settings::pitch::kSnapToKey);
                m_showPitchTextOverlay = settings.get(dstools::settings::pitch::kShowPitchTextOverlay);
                m_showPhonemeTexts = settings.get(dstools::settings::pitch::kShowPhonemeTexts);
                m_showCrosshairAndPitch = settings.get(dstools::settings::pitch::kShowCrosshairAndPitch);
                m_vScale = settings.get(dstools::settings::pitch::kVScale);
                m_boundaryHitRadius = settings.get(dstools::settings::pitch::kBoundaryHitRadius);
                m_defaultResolution = settings.get(dstools::settings::pitch::kDefaultResolution);
                m_inputHandler.setModulationDragSensitivity(settings.get(dstools::settings::pitch::kModulationDragSensitivity));

                if (m_actSnapToKey)
                    m_actSnapToKey->setChecked(m_snapToKey);
                if (m_actShowPitchTextOverlay)
                    m_actShowPitchTextOverlay->setChecked(m_showPitchTextOverlay);
                if (m_actShowPhonemeTexts)
                    m_actShowPhonemeTexts->setChecked(m_showPhonemeTexts);
                if (m_actShowCrosshairAndPitch)
                    m_actShowCrosshairAndPitch->setChecked(m_showCrosshairAndPitch);

                update();
            }

            void PianoRollView::pullConfig(dstools::AppSettings &settings) const {
                settings.set(dstools::settings::pitch::kSnapToKey, m_snapToKey);
                settings.set(dstools::settings::pitch::kShowPitchTextOverlay, m_showPitchTextOverlay);
                settings.set(dstools::settings::pitch::kShowPhonemeTexts, m_showPhonemeTexts);
                settings.set(dstools::settings::pitch::kShowCrosshairAndPitch, m_showCrosshairAndPitch);
                settings.set(dstools::settings::pitch::kVScale, m_vScale);
                settings.set(dstools::settings::pitch::kBoundaryHitRadius, m_boundaryHitRadius);
                settings.set(dstools::settings::pitch::kDefaultResolution, m_defaultResolution);
            }

            // ============================================================================
            // ViewportController integration
            // ============================================================================

            void PianoRollView::setViewportController(dsfw::widgets::ViewportController *vc) {
                if (!vc) {
                    qWarning() << "PianoRollView::setViewportController called with nullptr";
                    return;
                }
                m_viewport = vc;
                connect(vc, &dsfw::widgets::ViewportController::viewportChanged, this,
                        &PianoRollView::onViewportChanged, Qt::UniqueConnection);
            }

             void PianoRollView::updateCoord() {
                double startSec = m_viewport ? m_viewport->startSec() : 0.0;
                double endSec = m_viewport ? m_viewport->endSec() : startSec + 1.0;
                m_coord.viewStart = startSec;
                m_coord.viewEnd = endSec;
                m_coord.pixelWidth = width();
                m_coord.vScale = m_vScale;
                m_coord.contentTop = 0;
                m_coord.scrollX = 0.0;
                m_coord.scrollY = m_scrollY;
            }

            void PianoRollView::onViewportChanged(const dsfw::widgets::ViewportState &state) {
                Q_UNUSED(state)
                updateCoord();
                update();
            }

            // ============================================================================
            // Phoneme boundary model
            // ============================================================================

            void PianoRollView::setBoundaryModel(chart::IBoundaryModel *model) {
                m_phonemeBoundaryModel = model;
            }

            chart::IBoundaryModel *PianoRollView::boundaryModel() const {
                return m_phonemeBoundaryModel;
            }

            void PianoRollView::splitNoteAtBoundary(int noteIndex) {
                if (!m_dsFile || !m_undoStack || !m_phonemeBoundaryModel)
                    return;
                if (noteIndex < 0 || noteIndex >= static_cast<int>(m_dsFile->notes.size()))
                    return;

                const auto &note = m_dsFile->notes[noteIndex];
                TimePos noteStart = note.start;
                TimePos noteEnd = note.end();

                int activeTier = m_phonemeBoundaryModel->activeTierIndex();
                if (activeTier < 0)
                    return;

                std::vector<TimePos> splitTimes;
                int bc = m_phonemeBoundaryModel->boundaryCount(activeTier);
                for (int b = 0; b < bc; ++b) {
                    TimePos bt = m_phonemeBoundaryModel->boundaryTime(activeTier, b);
                    if (bt > noteStart && bt < noteEnd)
                        splitTimes.push_back(bt);
                }

                if (splitTimes.empty())
                    return;

                std::sort(splitTimes.begin(), splitTimes.end());

                // Reverse order: wait — we need to handle this carefully.
                // After each split, the note index changes. Split from right to left
                // using the original noteIndex for the last split.
                for (auto it = splitTimes.rbegin(); it != splitTimes.rend(); ++it)
                    m_undoStack->push(new SplitNoteCommand(m_dsFile, noteIndex, *it));

                update();
                emit noteSplitRequested(noteIndex);
            }

            void PianoRollView::snapAllNotesToPhoneme() {
                if (!m_dsFile || !m_undoStack || !m_phonemeBoundaryModel)
                    return;

                int activeTier = m_phonemeBoundaryModel->activeTierIndex();
                if (activeTier < 0)
                    return;

                int bc = m_phonemeBoundaryModel->boundaryCount(activeTier);
                if (bc < 1)
                    return;

                for (int n = static_cast<int>(m_dsFile->notes.size()) - 1; n >= 0; --n) {
                    const auto &note = m_dsFile->notes[n];
                    TimePos noteStart = note.start;
                    TimePos noteEnd = note.end();

                    bool hasBoundary = false;
                    for (int b = 0; b < bc; ++b) {
                        TimePos bt = m_phonemeBoundaryModel->boundaryTime(activeTier, b);
                        if (bt > noteStart && bt < noteEnd) {
                            hasBoundary = true;
                            break;
                        }
                    }

                    if (hasBoundary)
                        splitNoteAtBoundary(n);
                }

                emit snapToPhonemeRequested();
            }

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools