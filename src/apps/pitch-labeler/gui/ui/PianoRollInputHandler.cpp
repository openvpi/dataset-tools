#include "PianoRollInputHandler.h"
#include "PitchProcessor.h"
#include "gui/DSFile.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QUndoStack>
#include <QFrame>

#include "commands/PitchCommands.h"
#include "commands/NoteCommands.h"

#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>

#include <cmath>
#include <algorithm>

namespace dstools {
namespace pitchlabeler {
namespace ui {

using dstools::parseNoteName;
using dstools::shiftNoteCents;

PianoRollInputHandler::PianoRollInputHandler() = default;

void PianoRollInputHandler::reset() {
    m_isDragging = false;
    m_dragButton = Qt::NoButton;
    m_draggingNote = false;
    m_dragAccumulatedCents = 0;
    m_dragOrigMidi.clear();
    m_modulationDragging = false;
    m_driftDragging = false;
    m_preAdjustF0.clear();
    m_rubberBandActive = false;
    m_rubberBandRect = QRect();
    m_rulerDragging = false;
}

// ============================================================================
// Wheel
// ============================================================================

void PianoRollInputHandler::handleWheel(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
        double sceneX = m_cb.widgetXToScene(static_cast<int>(event->position().x()));
        double centerSec = m_cb.xToTime(sceneX);
        m_cb.viewportZoomAt(centerSec, factor);
    } else if (event->modifiers() & Qt::ShiftModifier) {
        double hScale = m_cb.timeToX(1.0) - m_cb.timeToX(0.0); // derive hScale
        double deltaSec = -event->angleDelta().y() / hScale;
        m_cb.viewportScrollBy(deltaSec);
    } else {
        m_cb.setVScrollValue(m_cb.getVScrollValue() - event->angleDelta().y());
    }
    event->accept();
}

// ============================================================================
// Mouse Press
// ============================================================================

void PianoRollInputHandler::handleMousePress(
    QMouseEvent *event, ToolMode toolMode,
    const std::shared_ptr<DSFile> &dsFile,
    std::set<int> &selectedNotes)
{
    if (event->button() == Qt::MiddleButton) {
        m_isDragging = true;
        m_dragStart = event->pos();
        m_dragButton = event->button();
        m_cb.setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton &&
        (event->modifiers() & Qt::AltModifier)) {
        m_isDragging = true;
        m_dragStart = event->pos();
        m_dragButton = Qt::MiddleButton;
        m_cb.setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();

        if (y < RulerHeight) {
            double sceneX = m_cb.widgetXToScene(x);
            double time = m_cb.xToTime(sceneX);
            if (time >= 0) {
                m_rulerDragging = true;
                m_cb.emitRulerClicked(time);
                m_cb.update();
            }
            return;
        }
        if (x < PianoWidth) return;

        int noteIdx = m_cb.getNoteAtPosition(x, y);

        if (noteIdx >= 0) {
            if (event->modifiers() & Qt::ControlModifier) {
                auto newSel = selectedNotes;
                if (newSel.count(noteIdx))
                    newSel.erase(noteIdx);
                else
                    newSel.insert(noteIdx);
                m_cb.selectNotes(newSel);
            } else if (selectedNotes.count(noteIdx) == 0) {
                m_cb.selectNotes({noteIdx});
            }
            m_cb.emitNoteSelected(noteIdx);

            // Modulation/Drift tool
            if (toolMode != ToolSelect && selectedNotes.count(noteIdx)) {
                double sceneY = m_cb.widgetYToScene(y);
                m_modulationDragStartY = sceneY;
                m_driftDragStartY = sceneY;
                m_modulationDragAmount = 1.0;
                m_driftDragAmount = 1.0;
                if (toolMode == ToolModulation) {
                    m_modulationDragging = true;
                } else {
                    m_driftDragging = true;
                }
                if (dsFile) {
                    m_preAdjustF0 = dsFile->f0.values;
                }
                m_cb.setCursor(Qt::SizeVerCursor);
                return;
            }

            // Select tool: start pitch drag
            if (selectedNotes.count(noteIdx) &&
                !(event->modifiers() & Qt::ControlModifier)) {
                m_draggingNote = true;
                double sceneY = m_cb.widgetYToScene(y);
                m_dragStartMidi = m_cb.yToMidi(sceneY);
                m_dragAccumulatedCents = 0;
                m_dragOrigMidi.clear();
                if (dsFile) {
                    for (int idx : selectedNotes) {
                        if (idx < static_cast<int>(dsFile->notes.size())) {
                            const auto &n = dsFile->notes[idx];
                            if (!n.isRest()) {
                                auto pitch = parseNoteName(n.name);
                                if (pitch.valid)
                                    m_dragOrigMidi[idx] = pitch.midiNumber;
                            }
                        }
                    }
                }
                m_cb.setCursor(Qt::SizeVerCursor);
            }
        } else {
            if (event->modifiers() & Qt::ControlModifier) {
                m_rubberBandActive = true;
                m_rubberBandStart = event->pos();
                m_rubberBandRect = QRect();
                m_cb.setCursor(Qt::CrossCursor);
            } else {
                m_cb.selectNotes({});
                double sceneX = m_cb.widgetXToScene(x);
                double sceneY = m_cb.widgetYToScene(y);
                m_cb.emitPositionClicked(m_cb.xToTime(sceneX), m_cb.yToMidi(sceneY));
            }
        }
    }
}

// ============================================================================
// Mouse Move
// ============================================================================

void PianoRollInputHandler::handleMouseMove(
    QMouseEvent *event, ToolMode /*toolMode*/,
    const std::shared_ptr<DSFile> &dsFile,
    const std::set<int> &selectedNotes,
    bool showCrosshairAndPitch)
{
    m_mousePos = event->pos();

    if (m_rulerDragging) {
        double sceneX = m_cb.widgetXToScene(event->pos().x());
        double time = std::max(0.0, m_cb.xToTime(sceneX));
        m_cb.emitRulerClicked(time);
        m_cb.update();
        return;
    }

    double sceneY = m_cb.widgetYToScene(event->pos().y());

    // Modulation drag
    if (m_modulationDragging && !selectedNotes.empty() && !m_preAdjustF0.empty()) {
        double dy = m_modulationDragStartY - sceneY;
        m_modulationDragAmount = std::clamp(1.0 + dy / ModulationDragSensitivity, 0.0, 5.0);
        if (dsFile) {
            PitchProcessor::applyModulationDriftPreview(
                *dsFile, m_preAdjustF0, selectedNotes,
                m_modulationDragAmount, m_driftDragAmount);
        }
        m_cb.update();
        return;
    }

    // Drift drag
    if (m_driftDragging && !selectedNotes.empty() && !m_preAdjustF0.empty()) {
        double dy = m_driftDragStartY - sceneY;
        m_driftDragAmount = std::clamp(1.0 + dy / ModulationDragSensitivity, 0.0, 5.0);
        if (dsFile) {
            PitchProcessor::applyModulationDriftPreview(
                *dsFile, m_preAdjustF0, selectedNotes,
                m_modulationDragAmount, m_driftDragAmount);
        }
        m_cb.update();
        return;
    }

    // Note pitch drag
    if (m_draggingNote && !selectedNotes.empty()) {
        double currentMidi = m_cb.yToMidi(sceneY);
        double deltaMidi = currentMidi - m_dragStartMidi;
        int deltaCents = static_cast<int>(deltaMidi * 100);
        if (deltaCents != m_dragAccumulatedCents) {
            m_dragAccumulatedCents = deltaCents;
            m_cb.update();
        }
        return;
    }

    // Panning
    if (m_isDragging && m_dragButton == Qt::MiddleButton) {
        QPoint delta = event->pos() - m_dragStart;
        double hScale = m_cb.timeToX(1.0) - m_cb.timeToX(0.0);
        double deltaSec = -delta.x() / hScale;
        m_cb.viewportScrollBy(deltaSec);
        m_cb.setVScrollValue(m_cb.getVScrollValue() - delta.y());
        m_dragStart = event->pos();
        return;
    }

    // Rubber-band
    if (m_rubberBandActive) {
        int x1 = std::min(m_rubberBandStart.x(), event->pos().x());
        int y1 = std::min(m_rubberBandStart.y(), event->pos().y());
        int x2 = std::max(m_rubberBandStart.x(), event->pos().x());
        int y2 = std::max(m_rubberBandStart.y(), event->pos().y());
        m_rubberBandRect = QRect(x1, y1, x2 - x1, y2 - y1);
        m_cb.update();
        return;
    }

    if (showCrosshairAndPitch) {
        m_cb.update();
    }
}

// ============================================================================
// Mouse Release
// ============================================================================

void PianoRollInputHandler::handleMouseRelease(
    QMouseEvent *event, ToolMode /*toolMode*/,
    const std::shared_ptr<DSFile> &dsFile,
    std::set<int> &selectedNotes,
    QUndoStack *undoStack)
{
    if (event->button() == Qt::MiddleButton) {
        m_isDragging = false;
        m_dragButton = Qt::NoButton;
        m_cb.restoreToolCursor();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (m_rulerDragging) {
            m_rulerDragging = false;
            return;
        }

        if (m_draggingNote) {
            m_draggingNote = false;
            int delta = m_dragAccumulatedCents;
            if (delta != 0 && dsFile && !selectedNotes.empty()) {
                std::vector<int> indices(selectedNotes.begin(), selectedNotes.end());
                m_cb.doPitchMove(indices, delta);
            }
            m_dragAccumulatedCents = 0;
            m_dragStartMidi = 0.0;
            m_dragOrigMidi.clear();
            m_cb.restoreToolCursor();
            m_cb.update();
            return;
        }

        if (m_modulationDragging || m_driftDragging) {
            if (undoStack && dsFile && !m_preAdjustF0.empty()) {
                undoStack->push(new ModulationDriftCommand(
                    dsFile, m_preAdjustF0, dsFile->f0.values));
            } else if (dsFile) {
                dsFile->markModified();
            }
            m_modulationDragging = false;
            m_driftDragging = false;
            m_preAdjustF0.clear();
            m_cb.restoreToolCursor();
            m_cb.emitFileEdited();
            return;
        }

        if (m_rubberBandActive) {
            m_rubberBandActive = false;
            if (!m_rubberBandRect.isNull() && dsFile) {
                std::set<int> selected;
                for (int i = 0; i < static_cast<int>(dsFile->notes.size()); ++i) {
                    const auto &note = dsFile->notes[i];
                    double noteMidi;
                    if (note.isRest()) {
                        noteMidi = PitchProcessor::getRestMidi(*dsFile, i);
                    } else {
                        auto pitch = parseNoteName(note.name);
                        if (!pitch.valid) continue;
                        noteMidi = pitch.midiNumber;
                    }
                    double sceneX1 = m_cb.timeToX(usToSec(note.start));
                    double sceneX2 = m_cb.timeToX(usToSec(note.end()));
                    double sceneY = m_cb.midiToY(noteMidi);
                    int wx1 = m_cb.sceneXToWidget(sceneX1);
                    int wx2 = m_cb.sceneXToWidget(sceneX2);
                    int wy = m_cb.sceneYToWidget(sceneY);
                    // Need vScale — derive from midiToY
                    double vScale = m_cb.midiToY(0) - m_cb.midiToY(1);
                    QRect noteRect(wx1, wy, wx2 - wx1, static_cast<int>(vScale));
                    if (m_rubberBandRect.intersects(noteRect)) {
                        selected.insert(i);
                    }
                }
                m_cb.selectNotes(selected);
            }
            m_rubberBandRect = QRect();
            m_cb.restoreToolCursor();
            m_cb.update();
            return;
        }

        m_isDragging = false;
        m_dragButton = Qt::NoButton;
        m_cb.restoreToolCursor();
    }
}

// ============================================================================
// Double Click
// ============================================================================

void PianoRollInputHandler::handleMouseDoubleClick(
    QMouseEvent *event,
    const std::shared_ptr<DSFile> &dsFile,
    std::set<int> &selectedNotes,
    QUndoStack *undoStack)
{
    if (event->button() == Qt::LeftButton) {
        int x = event->pos().x();
        int y = event->pos().y();
        if (x < PianoWidth || y < RulerHeight) return;

        int clicked = m_cb.getNoteAtPosition(x, y);
        if (clicked >= 0 && dsFile && !selectedNotes.empty()) {
            if (selectedNotes.count(clicked) == 0)
                m_cb.selectNotes({clicked});
            std::vector<int> indices(selectedNotes.begin(), selectedNotes.end());
            std::vector<QString> oldNames;
            bool hasSnap = false;
            for (int idx : indices) {
                if (idx < static_cast<int>(dsFile->notes.size())) {
                    oldNames.push_back(dsFile->notes[idx].name);
                    if (!dsFile->notes[idx].isRest()) {
                        auto pitch = parseNoteName(dsFile->notes[idx].name);
                        if (pitch.valid && pitch.cents != 0)
                            hasSnap = true;
                    }
                }
            }
            if (hasSnap && undoStack) {
                undoStack->push(new SnapToPitchCommand(dsFile, indices, oldNames));
            }
            m_cb.emitFileEdited();
            m_cb.update();
        }
    }
}

// ============================================================================
// Key Press
// ============================================================================

void PianoRollInputHandler::handleKeyPress(
    QKeyEvent *event,
    const std::shared_ptr<DSFile> &dsFile,
    const std::set<int> &selectedNotes)
{
    int key = event->key();
    auto mods = event->modifiers();

    if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        event->ignore();
        return;
    }

    if (key == Qt::Key_A && (mods & Qt::ControlModifier)) {
        m_cb.selectAllNotes();
        event->accept();
        return;
    }

    if (key == Qt::Key_Escape) {
        m_cb.clearSelection();
        event->accept();
        return;
    }

    if (!dsFile || selectedNotes.empty()) {
        event->ignore();
        return;
    }

    std::vector<int> indices(selectedNotes.begin(), selectedNotes.end());

    if (key == Qt::Key_Up) {
        int delta = 1;
        if (mods & Qt::ControlModifier) delta = 100;
        else if (mods & Qt::ShiftModifier) delta = 10;
        m_cb.doPitchMove(indices, delta);
        event->accept();
        return;
    }
    if (key == Qt::Key_Down) {
        int delta = -1;
        if (mods & Qt::ControlModifier) delta = -100;
        else if (mods & Qt::ShiftModifier) delta = -10;
        m_cb.doPitchMove(indices, delta);
        event->accept();
        return;
    }

    if (key == Qt::Key_Delete) {
        m_cb.emitNoteDeleteRequested(indices);
        event->accept();
        return;
    }

    event->ignore();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
