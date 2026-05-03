/// @file AudioFileListPanel.h
/// @brief Audio file list panel for DsSlicerPage — supports drag-drop and manual add/remove.

#pragma once

#include <QListWidget>
#include <QWidget>

class QVBoxLayout;
class QPushButton;

namespace dstools {

/// @brief Displays a list of audio files for slicing.
///
/// Supports adding files via button, menu, or drag-and-drop (files and directories).
/// Emits fileSelected when the user clicks on a file.
class AudioFileListPanel : public QWidget {
    Q_OBJECT

public:
    explicit AudioFileListPanel(QWidget *parent = nullptr);
    ~AudioFileListPanel() override = default;

    /// Add audio files to the list (deduplicates).
    void addFiles(const QStringList &paths);

    /// Add all audio files from a directory (non-recursive).
    void addDirectory(const QString &dirPath);

    /// Return all file paths in the list.
    [[nodiscard]] QStringList filePaths() const;

    /// Return the currently selected file path.
    [[nodiscard]] QString currentFilePath() const;

    /// File count.
    [[nodiscard]] int fileCount() const;

signals:
    void fileSelected(const QString &filePath);
    void filesAdded(const QStringList &paths);

private:
    QListWidget *m_listWidget = nullptr;
    QPushButton *m_btnAdd = nullptr;
    QPushButton *m_btnAddDir = nullptr;
    QPushButton *m_btnRemove = nullptr;

    void onAddFiles();
    void onAddDirectory();
    void onRemoveSelected();
    void onCurrentRowChanged(int row);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
};

} // namespace dstools
