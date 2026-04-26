#pragma once

#include <QString>
#include <QUndoCommand>
#include <dstools/TimePos.h>
#include <memory>
#include <vector>
#include <dstools/DsPitchDocument.h>

namespace dstools {
    namespace pitchlabeler {


        namespace ui {

            class SplitNoteCommand : public QUndoCommand {
            public:
                SplitNoteCommand(std::shared_ptr<DsPitchDocument> dsFile, int noteIndex, TimePos splitTime,
                                 QUndoCommand *parent = nullptr);

                void undo() override;
                void redo() override;

            private:
                std::shared_ptr<DsPitchDocument> m_dsFile;
                int m_noteIndex;
                TimePos m_splitTime;
                struct NoteSnapshot {
                    QString name;
                    TimePos duration;
                    int slur;
                    QString glide;
                };
                NoteSnapshot m_originalSnapshot;
            };

            class MergeNotesCommand : public QUndoCommand {
            public:
                MergeNotesCommand(std::shared_ptr<DsPitchDocument> dsFile, std::vector<int> indices,
                                  QUndoCommand *parent = nullptr);

                void undo() override;
                void redo() override;

            private:
                std::shared_ptr<DsPitchDocument> m_dsFile;
                int m_firstIndex;
                struct NoteSnapshot {
                    QString name;
                    TimePos duration;
                    int slur;
                    QString glide;
                };
                std::vector<NoteSnapshot> m_originalSnapshots;
                NoteSnapshot m_mergedSnapshot;
            };

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools