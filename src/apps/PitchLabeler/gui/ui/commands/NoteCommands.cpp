#include "NoteCommands.h"

#include "../../DSFile.h"
#include <dstools/PitchUtils.h>

namespace dstools {
namespace pitchlabeler {
namespace ui {

using dstools::parseNoteName;
using dstools::shiftNoteCents;

DeleteNotesCommand::DeleteNotesCommand(std::shared_ptr<DSFile> dsFile, std::vector<int> indices,
                                       QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_indices(std::move(indices)) {
    setText(QObject::tr("Delete notes"));
    for (int idx : m_indices) {
        if (idx >= 0 && idx < static_cast<int>(m_dsFile->notes.size())) {
            const auto &n = m_dsFile->notes[idx];
            m_snapshots.push_back({n.name, n.duration, n.slur, n.glide});
        }
    }
}

void DeleteNotesCommand::redo() {
    if (!m_dsFile) return;
    std::vector<int> sorted = m_indices;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int idx : sorted) {
        if (idx >= 0 && idx < static_cast<int>(m_dsFile->notes.size())) {
            m_dsFile->notes.erase(m_dsFile->notes.begin() + idx);
        }
    }
    m_dsFile->recomputeNoteStarts();
    m_dsFile->markModified();
}

void DeleteNotesCommand::undo() {
    if (!m_dsFile) return;
    for (size_t i = 0; i < m_indices.size() && i < m_snapshots.size(); ++i) {
        int idx = m_indices[i];
        Note n;
        n.name = m_snapshots[i].name;
        n.duration = m_snapshots[i].duration;
        n.slur = m_snapshots[i].slur;
        n.glide = m_snapshots[i].glide;
        n.start = 0;
        if (idx >= 0 && idx <= static_cast<int>(m_dsFile->notes.size())) {
            m_dsFile->notes.insert(m_dsFile->notes.begin() + idx, n);
        }
    }
    m_dsFile->recomputeNoteStarts();
    m_dsFile->markModified();
}

SetNoteGlideCommand::SetNoteGlideCommand(std::shared_ptr<DSFile> dsFile, int idx,
                                         QString oldGlide, QString newGlide,
                                         QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_idx(idx),
      m_oldGlide(std::move(oldGlide)), m_newGlide(std::move(newGlide)) {
    setText(QObject::tr("Change glide"));
}

void SetNoteGlideCommand::redo() {
    if (!m_dsFile || m_idx < 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    m_dsFile->notes[m_idx].glide = m_newGlide;
    m_dsFile->markModified();
}

void SetNoteGlideCommand::undo() {
    if (!m_dsFile || m_idx < 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    m_dsFile->notes[m_idx].glide = m_oldGlide;
    m_dsFile->markModified();
}

ToggleNoteSlurCommand::ToggleNoteSlurCommand(std::shared_ptr<DSFile> dsFile, int idx,
                                             int oldSlur, QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_idx(idx), m_oldSlur(oldSlur) {
    setText(QObject::tr("Toggle slur"));
}

void ToggleNoteSlurCommand::redo() {
    if (!m_dsFile || m_idx < 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    m_dsFile->notes[m_idx].slur = m_dsFile->notes[m_idx].slur ? 0 : 1;
    m_dsFile->markModified();
}

void ToggleNoteSlurCommand::undo() {
    if (!m_dsFile || m_idx < 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    m_dsFile->notes[m_idx].slur = m_oldSlur;
    m_dsFile->markModified();
}

ToggleNoteRestCommand::ToggleNoteRestCommand(std::shared_ptr<DSFile> dsFile, int idx,
                                             QString oldName, bool wasRest,
                                             QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_idx(idx),
      m_oldName(std::move(oldName)), m_wasRest(wasRest) {
    setText(QObject::tr("Toggle rest"));
}

void ToggleNoteRestCommand::redo() {
    if (!m_dsFile || m_idx < 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    auto &note = m_dsFile->notes[m_idx];
    if (note.isRest()) {
        bool found = false;
        for (int off = 1; off < static_cast<int>(m_dsFile->notes.size()) && !found; ++off) {
            for (int j : {m_idx - off, m_idx + off}) {
                if (j >= 0 && j < static_cast<int>(m_dsFile->notes.size()) &&
                    !m_dsFile->notes[j].isRest()) {
                    note.name = m_dsFile->notes[j].name;
                    found = true;
                    break;
                }
            }
        }
        if (!found) note.name = "C4";
    } else {
        note.name = "rest";
    }
    m_dsFile->markModified();
}

void ToggleNoteRestCommand::undo() {
    if (!m_dsFile || m_idx < 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    m_dsFile->notes[m_idx].name = m_oldName;
    m_dsFile->markModified();
}

MergeNoteLeftCommand::MergeNoteLeftCommand(std::shared_ptr<DSFile> dsFile, int idx,
                                           QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_idx(idx) {
    setText(QObject::tr("Merge note left"));
    if (m_dsFile && m_idx > 0 && m_idx < static_cast<int>(m_dsFile->notes.size())) {
        const auto &left = m_dsFile->notes[m_idx - 1];
        m_leftSnapshot = {left.name, left.duration, left.slur, left.glide};
        const auto &cur = m_dsFile->notes[m_idx];
        m_mergedSnapshot = {cur.name, cur.duration, cur.slur, cur.glide};
    }
}

void MergeNoteLeftCommand::redo() {
    if (!m_dsFile || m_idx <= 0 || m_idx >= static_cast<int>(m_dsFile->notes.size())) return;
    auto &left = m_dsFile->notes[m_idx - 1];
    auto &cur = m_dsFile->notes[m_idx];
    left.duration += cur.duration;
    m_dsFile->notes.erase(m_dsFile->notes.begin() + m_idx);
    m_dsFile->recomputeNoteStarts();
    m_dsFile->markModified();
}

void MergeNoteLeftCommand::undo() {
    if (!m_dsFile || m_idx <= 0 || m_idx > static_cast<int>(m_dsFile->notes.size())) return;
    Note restored;
    restored.name = m_mergedSnapshot.name;
    restored.duration = m_mergedSnapshot.duration;
    restored.slur = m_mergedSnapshot.slur;
    restored.glide = m_mergedSnapshot.glide;
    restored.start = 0;
    m_dsFile->notes.insert(m_dsFile->notes.begin() + m_idx, restored);
    m_dsFile->notes[m_idx - 1].name = m_leftSnapshot.name;
    m_dsFile->notes[m_idx - 1].duration = m_leftSnapshot.duration;
    m_dsFile->notes[m_idx - 1].slur = m_leftSnapshot.slur;
    m_dsFile->notes[m_idx - 1].glide = m_leftSnapshot.glide;
    m_dsFile->recomputeNoteStarts();
    m_dsFile->markModified();
}

SnapToPitchCommand::SnapToPitchCommand(std::shared_ptr<DSFile> dsFile, std::vector<int> indices,
                                       std::vector<QString> oldNames,
                                       QUndoCommand *parent)
    : QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_indices(std::move(indices)),
      m_oldNames(std::move(oldNames)) {
    setText(QObject::tr("Snap to pitch"));
    m_newNames.reserve(m_indices.size());
    for (size_t i = 0; i < m_indices.size() && i < m_oldNames.size(); ++i) {
        auto pitch = parseNoteName(m_oldNames[i]);
        if (pitch.valid && pitch.cents != 0) {
            m_newNames.push_back(shiftNoteCents(m_oldNames[i], -pitch.cents));
        } else {
            m_newNames.push_back(m_oldNames[i]);
        }
    }
}

void SnapToPitchCommand::redo() {
    if (!m_dsFile) return;
    for (size_t i = 0; i < m_indices.size() && i < m_newNames.size(); ++i) {
        int idx = m_indices[i];
        if (idx >= 0 && idx < static_cast<int>(m_dsFile->notes.size())) {
            m_dsFile->notes[idx].name = m_newNames[i];
        }
    }
    m_dsFile->markModified();
}

void SnapToPitchCommand::undo() {
    if (!m_dsFile) return;
    for (size_t i = 0; i < m_indices.size() && i < m_oldNames.size(); ++i) {
        int idx = m_indices[i];
        if (idx >= 0 && idx < static_cast<int>(m_dsFile->notes.size())) {
            m_dsFile->notes[idx].name = m_oldNames[i];
        }
    }
    m_dsFile->markModified();
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
