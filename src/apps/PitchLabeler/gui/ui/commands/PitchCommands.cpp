#include "PitchCommands.h"

#include "../../DSFile.h"
#include <dstools/PitchUtils.h>

namespace dstools {
namespace pitchlabeler {
namespace ui {

// ============================================================================
// PitchMoveCommand
// ============================================================================

PitchMoveCommand::PitchMoveCommand(std::shared_ptr<DSFile> dsFile, std::vector<int> indices,
                                   int deltaCents, QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_indices(std::move(indices)),
      m_deltaCents(deltaCents) {
    setText(QObject::tr("Move pitch %1 cents").arg(deltaCents));
}

void PitchMoveCommand::undo() {
    if (!m_dsFile) return;
    for (int idx : m_indices) {
        if (idx < 0 || idx >= static_cast<int>(m_dsFile->notes.size())) continue;
        auto &note = m_dsFile->notes[idx];
        if (note.isRest()) continue;
        note.name = shiftNoteCents(note.name, -m_deltaCents);
    }
    m_dsFile->markModified();
}

void PitchMoveCommand::redo() {
    if (!m_dsFile) return;
    for (int idx : m_indices) {
        if (idx < 0 || idx >= static_cast<int>(m_dsFile->notes.size())) continue;
        auto &note = m_dsFile->notes[idx];
        if (note.isRest()) continue;
        note.name = shiftNoteCents(note.name, m_deltaCents);
    }
    m_dsFile->markModified();
}

bool PitchMoveCommand::mergeWith(const QUndoCommand *other) {
    if (other->id() != id()) return false;
    auto *cmd = static_cast<const PitchMoveCommand *>(other);
    if (cmd->m_indices != m_indices) return false;
    m_deltaCents += cmd->m_deltaCents;
    setText(QObject::tr("Move pitch %1 cents").arg(m_deltaCents));
    return true;
}

// ============================================================================
// ModulationDriftCommand
// ============================================================================

ModulationDriftCommand::ModulationDriftCommand(std::shared_ptr<DSFile> dsFile,
                                               std::vector<double> oldF0Values,
                                               std::vector<double> newF0Values,
                                               QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)),
      m_oldF0(std::move(oldF0Values)), m_newF0(std::move(newF0Values)) {
    setText(QObject::tr("Adjust modulation/drift"));
}

void ModulationDriftCommand::undo() {
    if (!m_dsFile) return;
    m_dsFile->f0.values = m_oldF0;
    m_dsFile->markModified();
}

void ModulationDriftCommand::redo() {
    if (!m_dsFile) return;
    m_dsFile->f0.values = m_newF0;
    m_dsFile->markModified();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
