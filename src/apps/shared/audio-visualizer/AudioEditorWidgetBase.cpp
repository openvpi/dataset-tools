#include "AudioEditorWidgetBase.h"

namespace dstools {

AudioEditorWidgetBase::AudioEditorWidgetBase(QWidget *parent)
    : QWidget(parent)
    , m_undoStack(new QUndoStack(this))
    , m_actUndo(new QAction(this))
    , m_actRedo(new QAction(this))
{
}

void AudioEditorWidgetBase::connectUndoRedo() {
    connect(m_actUndo, &QAction::triggered, this, &AudioEditorWidgetBase::onUndo);
    connect(m_actRedo, &QAction::triggered, this, &AudioEditorWidgetBase::onRedo);
    connect(m_undoStack, &QUndoStack::canUndoChanged, m_actUndo, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::canRedoChanged, m_actRedo, &QAction::setEnabled);
}

void AudioEditorWidgetBase::onUndo() {
    m_undoStack->undo();
    updateUndoRedoState();
    emit modificationChanged(true);
}

void AudioEditorWidgetBase::onRedo() {
    m_undoStack->redo();
    updateUndoRedoState();
    emit modificationChanged(true);
}

void AudioEditorWidgetBase::updateUndoRedoState() {
    m_actUndo->setEnabled(m_undoStack->canUndo());
    m_actRedo->setEnabled(m_undoStack->canRedo());
}

} // namespace dstools