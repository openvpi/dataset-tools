#pragma once

#include <QUndoCommand>
#include <QString>
#include <dstools/TimePos.h>
#include <vector>
#include <memory>

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {

class DeleteNotesCommand : public QUndoCommand {
public:
    DeleteNotesCommand(std::shared_ptr<DSFile> dsFile, std::vector<int> indices,
                       QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    std::vector<int> m_indices;
    struct NoteSnapshot {
        QString name;
        TimePos duration;
        int slur;
        QString glide;
    };
    std::vector<NoteSnapshot> m_snapshots;
};

class SetNoteGlideCommand : public QUndoCommand {
public:
    SetNoteGlideCommand(std::shared_ptr<DSFile> dsFile, int idx, QString oldGlide, QString newGlide,
                        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    int m_idx;
    QString m_oldGlide;
    QString m_newGlide;
};

class ToggleNoteSlurCommand : public QUndoCommand {
public:
    ToggleNoteSlurCommand(std::shared_ptr<DSFile> dsFile, int idx, int oldSlur,
                          QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    int m_idx;
    int m_oldSlur;
};

class ToggleNoteRestCommand : public QUndoCommand {
public:
    ToggleNoteRestCommand(std::shared_ptr<DSFile> dsFile, int idx, QString oldName, bool wasRest,
                          QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    int m_idx;
    QString m_oldName;
    bool m_wasRest;
};

class MergeNoteLeftCommand : public QUndoCommand {
public:
    MergeNoteLeftCommand(std::shared_ptr<DSFile> dsFile, int idx,
                         QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    int m_idx;
    struct NoteSnapshot {
        QString name;
        TimePos duration;
        int slur;
        QString glide;
    };
    NoteSnapshot m_leftSnapshot;
    NoteSnapshot m_mergedSnapshot;
};

class SnapToPitchCommand : public QUndoCommand {
public:
    SnapToPitchCommand(std::shared_ptr<DSFile> dsFile, std::vector<int> indices,
                       std::vector<QString> oldNames,
                       QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    std::shared_ptr<DSFile> m_dsFile;
    std::vector<int> m_indices;
    std::vector<QString> m_oldNames;
    std::vector<QString> m_newNames;
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
