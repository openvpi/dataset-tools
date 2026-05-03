/// @file SliceListPanel.h
/// @brief Slice navigation panel for DsLabeler pages, driven by ProjectDataSource.

#pragma once

#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace dsfw::widgets {
class FileProgressTracker;
}

namespace dstools {

class ProjectDataSource;

/// @brief Displays project slices and emits selection changes.
///
/// Unlike BaseFileListPanel (filesystem-based), this panel is driven by
/// ProjectDataSource::sliceIds(). Slice status (active/discarded) is
/// reflected via item styling. Includes a bottom progress bar showing
/// labeling completion status (e.g. "12 / 50 已标注").
class SliceListPanel : public QWidget {
    Q_OBJECT

public:
    explicit SliceListPanel(QWidget *parent = nullptr);
    ~SliceListPanel() override = default;

    /// Set the data source and refresh the list.
    void setDataSource(ProjectDataSource *source);

    /// Refresh the slice list from the data source.
    void refresh();

    /// Select a slice by ID.
    void setCurrentSlice(const QString &sliceId);

    /// Return the currently selected slice ID.
    [[nodiscard]] QString currentSliceId() const;

    /// Navigate to next/previous slice.
    void selectNext();
    void selectPrev();

    /// Slice count.
    [[nodiscard]] int sliceCount() const;

    /// Update the labeling progress display.
    /// @param completed Number of slices that have been labeled/processed.
    /// @param total Total number of slices.
    void setProgress(int completed, int total);

    /// Access the progress tracker widget.
    [[nodiscard]] dsfw::widgets::FileProgressTracker *progressTracker() const;

signals:
    void sliceSelected(const QString &sliceId);

private:
    QListWidget *m_listWidget = nullptr;
    ProjectDataSource *m_source = nullptr;
    dsfw::widgets::FileProgressTracker *m_progressTracker = nullptr;

    void onCurrentRowChanged(int row);
};

} // namespace dstools
