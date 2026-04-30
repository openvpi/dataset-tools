#pragma once

/// @file IPageProgress.h
/// @brief Page progress interface for reporting batch operation or file review progress.

#include <QObject>
#include <QString>

namespace dstools::labeler {

/// @brief Page progress reporting interface.
///
/// Batch pages report processing progress; interactive pages report file review progress.
/// AppShell queries this interface to update the status bar.
class IPageProgress {
public:
    virtual ~IPageProgress() = default;

    /// @brief Return the total number of items to process.
    /// @return Total item count, or 0 if not applicable.
    virtual int progressTotal() const { return 0; }

    /// @brief Return the number of items processed so far.
    /// @return Processed item count.
    virtual int progressCurrent() const { return 0; }

    /// @brief Check whether a batch operation is currently running.
    /// @return True if running.
    virtual bool isRunning() const { return false; }

    /// @brief Return a human-readable status message for the status bar.
    /// @return Status message string.
    virtual QString progressMessage() const { return {}; }

    /// @brief Request cancellation of the current operation.
    virtual void cancelOperation() {}
};

} // namespace dstools::labeler

#define LABELER_PAGE_PROGRESS_IID "dstools.labeler.IPageProgress"
Q_DECLARE_INTERFACE(dstools::labeler::IPageProgress, LABELER_PAGE_PROGRESS_IID)
