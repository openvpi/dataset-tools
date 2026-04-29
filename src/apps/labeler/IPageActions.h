#pragma once

#include <QAction>
#include <QList>
#include <QString>

namespace dstools::labeler {

class IPageActions {
public:
    virtual ~IPageActions() = default;

    virtual QList<QAction *> editActions() const { return {}; }
    virtual QList<QAction *> viewActions() const { return {}; }
    virtual QList<QAction *> toolActions() const { return {}; }

    virtual bool hasUnsavedChanges() const { return false; }
    virtual bool save() { return true; }

    virtual void setWorkingDirectory(const QString &) {}
    virtual QString workingDirectory() const { return {}; }
};

} // namespace dstools::labeler

#define LABELER_PAGE_ACTIONS_IID "dstools.labeler.IPageActions"
Q_DECLARE_INTERFACE(dstools::labeler::IPageActions, LABELER_PAGE_ACTIONS_IID)
