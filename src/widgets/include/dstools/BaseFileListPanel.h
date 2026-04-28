#pragma once

#include <dstools/WidgetsGlobal.h>

#include <QWidget>
#include <QStringList>

class QListWidget;
class QListWidgetItem;

namespace dstools::widgets {

class FileProgressTracker;

/// Base class for file list panels.
/// Provides directory scanning, file filtering, selection, navigation.
/// Subclasses customize file extension filters and item styling.
class DSTOOLS_WIDGETS_API BaseFileListPanel : public QWidget {
    Q_OBJECT
public:
    explicit BaseFileListPanel(QWidget *parent = nullptr);
    ~BaseFileListPanel() override;

    /// Set working directory and refresh file list.
    virtual void setDirectory(const QString &path);

    /// Set file name filters, e.g. {"*.ds"} or {"*.TextGrid", "*.textgrid"}.
    void setFileFilters(const QStringList &filters);

    /// Enable/disable progress tracker display.
    void setShowProgress(bool show);

    /// Access internal QListWidget for subclass customization.
    QListWidget *listWidget() const { return m_listWidget; }

    /// Current selected file full path (empty if none).
    QString currentFile() const;

    /// Select file by full path or filename.
    void setCurrentFile(const QString &path);

    /// Navigate to next/previous file.
    void selectNextFile();
    void selectPrevFile();

    /// File count.
    int fileCount() const;

    /// Access the progress tracker (may be null if not shown).
    FileProgressTracker *progressTracker() const { return m_progressTracker; }

signals:
    void fileSelected(const QString &filePath);
    void fileListRefreshed(int count);

protected:
    /// Called for each file entry during refresh. Subclasses can customize item appearance.
    /// Default implementation just sets item text to filename.
    virtual void styleItem(QListWidgetItem *item, const QString &filePath);

    /// Refresh file list from current directory. Can be overridden for custom loading.
    virtual void refreshFileList();

    /// Access current directory path.
    QString directory() const { return m_directory; }

private:
    QListWidget *m_listWidget = nullptr;
    FileProgressTracker *m_progressTracker = nullptr;
    QString m_directory;
    QStringList m_filters;
    bool m_showProgress = false;

    void onCurrentRowChanged(int row);
};

} // namespace dstools::widgets
