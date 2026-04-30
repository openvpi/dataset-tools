#pragma once

#include <QObject>
#include <QString>

namespace dstools::labeler {

/// Page progress reporting interface.
/// Batch pages report progress; interactive pages report file review progress.
class IPageProgress {
public:
    virtual ~IPageProgress() = default;

    /// Total number of items to process
    virtual int progressTotal() const { return 0; }

    /// Number of items processed
    virtual int progressCurrent() const { return 0; }

    /// Whether a batch operation is currently running
    virtual bool isRunning() const { return false; }

    /// Human-readable status message for the status bar
    virtual QString progressMessage() const { return {}; }

    /// Request to cancel the current operation
    virtual void cancelOperation() {}
};

} // namespace dstools::labeler

#define LABELER_PAGE_PROGRESS_IID "dstools.labeler.IPageProgress/1.0"
Q_DECLARE_INTERFACE(dstools::labeler::IPageProgress, LABELER_PAGE_PROGRESS_IID)
