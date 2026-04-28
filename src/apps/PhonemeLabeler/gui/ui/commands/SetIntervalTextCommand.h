#pragma once

#include <QUndoCommand>
#include <QString>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class SetIntervalTextCommand : public QUndoCommand {
public:
    SetIntervalTextCommand(TextGridDocument *doc, int tierIndex,
                           int intervalIndex, const QString &oldText,
                           const QString &newText, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 2; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    TextGridDocument *m_doc;
    int m_tierIndex;
    int m_intervalIndex;
    QString m_oldText;
    QString m_newText;
};

} // namespace phonemelabeler
} // namespace dstools
