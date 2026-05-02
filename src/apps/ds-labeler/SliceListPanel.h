/// @file SliceListPanel.h
/// @brief Slice navigation panel for DsLabeler pages, driven by ProjectDataSource.

#pragma once

#include <QListWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace dstools {

class ProjectDataSource;

/// @brief Displays project slices and emits selection changes.
///
/// Unlike BaseFileListPanel (filesystem-based), this panel is driven by
/// ProjectDataSource::sliceIds(). Slice status (active/discarded) is
/// reflected via item styling.
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

signals:
    void sliceSelected(const QString &sliceId);

private:
    QListWidget *m_listWidget = nullptr;
    ProjectDataSource *m_source = nullptr;

    void onCurrentRowChanged(int row);
};

} // namespace dstools
