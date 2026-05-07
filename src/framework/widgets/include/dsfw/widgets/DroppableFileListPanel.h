#pragma once
/// @file DroppableFileListPanel.h
/// @brief Generic file list panel with drag-and-drop, configurable filters, and progress bar.

#include <dsfw/widgets/WidgetsGlobal.h>
#include <QWidget>
#include <QStringList>

class QListWidget;
class QListWidgetItem;
class QToolButton;

namespace dsfw::widgets {

class FileProgressTracker;

/// @brief Reusable file list panel supporting drag-and-drop, add/remove, directory import.
///
/// Configurable file type filters (e.g. {"*.wav", "*.flac"}). Includes an
/// integrated FileProgressTracker at the bottom for displaying labeling/processing
/// progress. Supports both file and directory drops.
///
/// Usage:
/// @code
///   auto *panel = new DroppableFileListPanel(this);
///   panel->setFileFilters({"*.wav", "*.flac", "*.mp3"});
///   panel->setShowProgress(true);
///   panel->progressTracker()->setFormat("%1 / %2 已标注");
/// @endcode
class DSFW_WIDGETS_API DroppableFileListPanel : public QWidget {
    Q_OBJECT

public:
    explicit DroppableFileListPanel(QWidget *parent = nullptr);
    ~DroppableFileListPanel() override;

    /// Set the accepted file name filters (e.g. {"*.wav", "*.flac"}).
    /// These are used for both directory scanning and drag-drop validation.
    void setFileFilters(const QStringList &filters);

    /// Get the current file filters.
    [[nodiscard]] QStringList fileFilters() const;

    /// Add files to the list (deduplicates, validates against filters).
    void addFiles(const QStringList &paths);

    /// Add all matching files from a directory (non-recursive).
    void addDirectory(const QString &dirPath);

    /// Remove all files from the list.
    void clear();

    /// Return all file paths in the list.
    [[nodiscard]] QStringList filePaths() const;

    /// Return the currently selected file path.
    [[nodiscard]] QString currentFilePath() const;

    /// Select a file by path.
    void setCurrentFile(const QString &path);

    /// Navigate to next/previous file.
    void selectNext();
    void selectPrev();

    /// File count.
    [[nodiscard]] int fileCount() const;

    /// Enable/disable the bottom progress tracker.
    void setShowProgress(bool show);

    /// Access the progress tracker widget.
    [[nodiscard]] FileProgressTracker *progressTracker() const;

    /// Access the internal list widget.
    [[nodiscard]] QListWidget *listWidget() const;

    /// Show or hide a button by name.
    /// Valid names: "addDir", "add", "remove", "discard", "clear".
    void setButtonVisible(const QString &name, bool visible);

signals:
    /// Emitted when a file is selected (by click).
    void fileSelected(const QString &filePath);

    /// Emitted after files are added.
    void filesAdded(const QStringList &paths);

    /// Emitted after files are removed.
    void filesRemoved();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    /// Called for each file during addFiles. Subclasses can customize item appearance.
    virtual void styleItem(QListWidgetItem *item, const QString &filePath);

private:
    QListWidget *m_listWidget = nullptr;
    QToolButton *m_btnAdd = nullptr;
    QToolButton *m_btnAddDir = nullptr;
    QToolButton *m_btnRemove = nullptr;
    QToolButton *m_btnDiscard = nullptr;
    QToolButton *m_btnClear = nullptr;
    FileProgressTracker *m_progressTracker = nullptr;
    QStringList m_filters;

    bool matchesFilter(const QString &filePath) const;
    void onAddFiles();
    void onAddDirectory();
    void onRemoveSelected();
    void onDiscardSelected();
    void onClearAll();
    void onCurrentRowChanged(int row);
    void updateDiscardButtonText();
};

} // namespace dsfw::widgets
