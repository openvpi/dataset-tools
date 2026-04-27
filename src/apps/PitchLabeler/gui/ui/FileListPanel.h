#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QString>
#include <QSet>

namespace dstools {
namespace pitchlabeler {
namespace ui {

/// File list panel showing .ds files in working directory.
/// Tracks three states per file:
///   - Unsaved (not yet opened/touched) — default color
///   - Modified (opened and changed, not saved) — orange indicator
///   - Saved (explicitly saved after modification) — green indicator
class FileListPanel : public QWidget {
    Q_OBJECT

public:
    explicit FileListPanel(QWidget *parent = nullptr);
    ~FileListPanel() override;

    void setDirectory(const QString &path);
    void clear();

    // Selection helpers
    void selectNextFile();
    void selectPrevFile();
    QString currentFilePath() const;

    // Modification state
    void setFileModified(const QString &path, bool modified);
    void setFileSaved(const QString &path);

    // Progress
    int totalFiles() const;
    int savedFiles() const;

    // Persistence
    void saveState();

signals:
    void fileSelected(const QString &path);

private:
    QListWidget *m_listWidget = nullptr;
    QLabel *m_progressLabel = nullptr;
    QString m_directory;
    QString m_lastFile;

    // Track file states
    QSet<QString> m_modifiedFiles;  // currently modified (unsaved)
    QSet<QString> m_savedFiles;     // saved at least once

    void populateList();
    void onItemClicked(QListWidgetItem *item);
    void onCurrentRowChanged(int row);
    void loadState();
    void updateItemStyle(QListWidgetItem *item, const QString &path);
    void updateProgressLabel();
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
