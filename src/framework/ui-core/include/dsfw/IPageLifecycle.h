#pragma once

/// @file IPageLifecycle.h
/// @brief Page lifecycle interface for activation, deactivation, and shutdown callbacks.

#include <QString>

namespace dstools::labeler {

/// @brief Page lifecycle interface.
///
/// Called by AppShell/LabelerWindow when the user switches steps or the application closes.
/// Implement on page widgets to respond to visibility and directory changes.
class IPageLifecycle {
public:
    virtual ~IPageLifecycle() = default;

    /// @brief Called when this page becomes the active (visible) page.
    virtual void onActivated() {}

    /// @brief Called when this page is about to become inactive.
    /// @return False to veto the switch and remain active.
    virtual bool onDeactivating() { return true; }

    /// @brief Called after the page has been deactivated.
    virtual void onDeactivated() {}

    /// @brief Called when the working directory changes.
    /// @param newDir New working directory path.
    virtual void onWorkingDirectoryChanged(const QString &newDir) {
        Q_UNUSED(newDir);
    }

    /// @brief Called before the page is destroyed (app closing).
    virtual void onShutdown() {}
};

} // namespace dstools::labeler

#define LABELER_PAGE_LIFECYCLE_IID "dstools.labeler.IPageLifecycle"
Q_DECLARE_INTERFACE(dstools::labeler::IPageLifecycle, LABELER_PAGE_LIFECYCLE_IID)
