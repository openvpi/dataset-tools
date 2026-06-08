#include "SplitMergeCommands.h"

#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {
        namespace ui {

            SplitNoteCommand::SplitNoteCommand(std::shared_ptr<DsPitchDocument> dsFile, int noteIndex, dsfw::TimePos splitTime,
                                               QUndoCommand *parent) :
                QUndoCommand(parent), m_dsFile(std::move(dsFile)), m_noteIndex(noteIndex), m_splitTime(splitTime) {
                setText(QObject::tr("Split note"));
                if (m_dsFile && m_noteIndex >= 0 && m_noteIndex < static_cast<int>(m_dsFile->notes.size())) {
                    const auto &n = m_dsFile->notes[m_noteIndex];
                    m_originalSnapshot = {n.name, n.duration, n.slur, n.glide};
                }
            }

            void SplitNoteCommand::redo() {
                if (!m_dsFile || m_noteIndex < 0 || m_noteIndex >= static_cast<int>(m_dsFile->notes.size()))
                    return;

                const auto &n = m_dsFile->notes[m_noteIndex];
                dsfw::TimePos noteStart = n.start;
                dsfw::TimePos noteEnd = noteStart + m_originalSnapshot.duration;

                if (m_splitTime <= noteStart || m_splitTime >= noteEnd)
                    return;

                Note left;
                left.name = m_originalSnapshot.name;
                left.duration = m_splitTime - noteStart;
                left.slur = m_originalSnapshot.slur;
                left.glide = m_originalSnapshot.glide;
                left.start = noteStart;

                Note right;
                right.name = m_originalSnapshot.name;
                right.duration = noteEnd - m_splitTime;
                right.slur = 0;
                right.glide = QString{};
                right.start = m_splitTime;

                m_dsFile->notes.erase(m_dsFile->notes.begin() + m_noteIndex);
                m_dsFile->notes.insert(m_dsFile->notes.begin() + m_noteIndex, right);
                m_dsFile->notes.insert(m_dsFile->notes.begin() + m_noteIndex, left);

                m_dsFile->recomputeNoteStarts();
                m_dsFile->markModified();
            }

            void SplitNoteCommand::undo() {
                if (!m_dsFile || m_noteIndex < 0 || m_noteIndex + 1 >= static_cast<int>(m_dsFile->notes.size()))
                    return;

                m_dsFile->notes.erase(m_dsFile->notes.begin() + m_noteIndex + 1);
                m_dsFile->notes.erase(m_dsFile->notes.begin() + m_noteIndex);

                Note restored;
                restored.name = m_originalSnapshot.name;
                restored.duration = m_originalSnapshot.duration;
                restored.slur = m_originalSnapshot.slur;
                restored.glide = m_originalSnapshot.glide;
                restored.start = 0;
                m_dsFile->notes.insert(m_dsFile->notes.begin() + m_noteIndex, restored);

                m_dsFile->recomputeNoteStarts();
                m_dsFile->markModified();
            }

            MergeNotesCommand::MergeNotesCommand(std::shared_ptr<DsPitchDocument> dsFile, std::vector<int> indices,
                                                 QUndoCommand *parent) :
                QUndoCommand(parent), m_dsFile(std::move(dsFile)) {
                setText(QObject::tr("Merge notes"));

                std::sort(indices.begin(), indices.end());
                m_firstIndex = indices.empty() ? -1 : indices.front();

                if (m_dsFile) {
                    for (int idx : indices) {
                        if (idx >= 0 && idx < static_cast<int>(m_dsFile->notes.size())) {
                            const auto &n = m_dsFile->notes[idx];
                            m_originalSnapshots.push_back({n.name, n.duration, n.slur, n.glide});
                        }
                    }
                }

                if (!m_originalSnapshots.empty()) {
                    m_mergedSnapshot = m_originalSnapshots.front();
                    m_mergedSnapshot.duration = 0;
                    for (const auto &snap : m_originalSnapshots)
                        m_mergedSnapshot.duration += snap.duration;
                }
            }

            void MergeNotesCommand::redo() {
                if (!m_dsFile || m_firstIndex < 0 || m_originalSnapshots.empty())
                    return;

                int count = static_cast<int>(m_originalSnapshots.size());
                if (m_firstIndex + count > static_cast<int>(m_dsFile->notes.size()))
                    return;

                for (int i = count - 1; i >= 0; --i)
                    m_dsFile->notes.erase(m_dsFile->notes.begin() + m_firstIndex + i);

                Note merged;
                merged.name = m_mergedSnapshot.name;
                merged.duration = m_mergedSnapshot.duration;
                merged.slur = m_mergedSnapshot.slur;
                merged.glide = m_mergedSnapshot.glide;
                merged.start = 0;
                m_dsFile->notes.insert(m_dsFile->notes.begin() + m_firstIndex, merged);

                m_dsFile->recomputeNoteStarts();
                m_dsFile->markModified();
            }

            void MergeNotesCommand::undo() {
                if (!m_dsFile || m_firstIndex < 0 || m_originalSnapshots.empty())
                    return;

                m_dsFile->notes.erase(m_dsFile->notes.begin() + m_firstIndex);

                for (size_t i = 0; i < m_originalSnapshots.size(); ++i) {
                    Note n;
                    n.name = m_originalSnapshots[i].name;
                    n.duration = m_originalSnapshots[i].duration;
                    n.slur = m_originalSnapshots[i].slur;
                    n.glide = m_originalSnapshots[i].glide;
                    n.start = 0;
                    m_dsFile->notes.insert(m_dsFile->notes.begin() + m_firstIndex + static_cast<int>(i), n);
                }

                m_dsFile->recomputeNoteStarts();
                m_dsFile->markModified();
            }

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools
