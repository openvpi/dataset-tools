#pragma once

#include <QAction>
#include <QList>
#include <QMenuBar>
#include <QString>

class QDragEnterEvent;
class QDropEvent;
class QWidget;

namespace dstools::labeler {

class IPageActions {
public:
    virtual ~IPageActions() = default;

    [[deprecated("Use createMenuBar() instead")]]
    virtual QList<QAction *> editActions() const { return {}; }
    [[deprecated("Use createMenuBar() instead")]]
    virtual QList<QAction *> viewActions() const { return {}; }
    [[deprecated("Use createMenuBar() instead")]]
    virtual QList<QAction *> toolActions() const { return {}; }

    virtual bool hasUnsavedChanges() const { return false; }
    virtual bool save() { return true; }

    virtual void setWorkingDirectory(const QString &) {}
    virtual QString workingDirectory() const { return {}; }

    virtual QMenuBar *createMenuBar(QWidget *parent) { Q_UNUSED(parent); return nullptr; }
    virtual QWidget *createStatusBarContent(QWidget *parent) { Q_UNUSED(parent); return nullptr; }
    virtual QString windowTitle() const { return {}; }
    virtual bool supportsDragDrop() const { return false; }
    virtual void handleDragEnter(QDragEnterEvent *event) { Q_UNUSED(event); }
    virtual void handleDrop(QDropEvent *event) { Q_UNUSED(event); }
};

} // namespace dstools::labeler

#define LABELER_PAGE_ACTIONS_IID "dstools.labeler.IPageActions/1.0"
Q_DECLARE_INTERFACE(dstools::labeler::IPageActions, LABELER_PAGE_ACTIONS_IID)
