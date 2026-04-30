#pragma once

#include <QString>

namespace dstools::labeler {

/// Page lifecycle interface.
/// Called by LabelerWindow when the user switches steps.
class IPageLifecycle {
public:
    virtual ~IPageLifecycle() = default;

    /// Called when this page becomes the active (visible) page.
    virtual void onActivated() {}

    /// Called when this page is about to become inactive.
    /// Return false to veto the switch.
    virtual bool onDeactivating() { return true; }

    /// Called after the page has been deactivated.
    virtual void onDeactivated() {}

    /// Called when the working directory changes.
    virtual void onWorkingDirectoryChanged(const QString &newDir) {
        Q_UNUSED(newDir);
    }

    /// Called before the page is destroyed (app closing).
    virtual void onShutdown() {}
};

} // namespace dstools::labeler

#define LABELER_PAGE_LIFECYCLE_IID "dstools.labeler.IPageLifecycle"
Q_DECLARE_INTERFACE(dstools::labeler::IPageLifecycle, LABELER_PAGE_LIFECYCLE_IID)
