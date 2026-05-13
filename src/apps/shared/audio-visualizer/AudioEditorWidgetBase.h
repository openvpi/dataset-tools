#pragma once

#include <QAction>
#include <QUndoStack>
#include <QWidget>

namespace dstools {

class AudioEditorWidgetBase : public QWidget {
    Q_OBJECT
public:
    explicit AudioEditorWidgetBase(QWidget *parent = nullptr);

    [[nodiscard]] QUndoStack *undoStack() const { return m_undoStack; }
    [[nodiscard]] QAction *undoAction() const { return m_actUndo; }
    [[nodiscard]] QAction *redoAction() const { return m_actRedo; }

protected:
    QUndoStack *m_undoStack = nullptr;
    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;

    void connectUndoRedo();

    virtual void onUndo();
    virtual void onRedo();
    virtual void updateUndoRedoState();

signals:
    void modificationChanged(bool modified);
};

} // namespace dstools