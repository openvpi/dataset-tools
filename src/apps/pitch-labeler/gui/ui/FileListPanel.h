#pragma once

#include <dstools/BaseFileListPanel.h>
#include <QSet>

namespace dstools {
namespace pitchlabeler {
namespace ui {

/// File list panel for .ds files with 3-color status tracking and state persistence.
class FileListPanel : public dstools::widgets::BaseFileListPanel {
    Q_OBJECT

public:
    explicit FileListPanel(QWidget *parent = nullptr);
    ~FileListPanel() override;

    void setDirectory(const QString &path) override;
    void clear();

    // Modification state
    void setFileModified(const QString &path, bool modified);
    void setFileSaved(const QString &path);

    // Progress
    int totalFiles() const;
    int savedFiles() const;

    // Persistence
    void saveState();

protected:
    void styleItem(QListWidgetItem *item, const QString &filePath) override;

private:
    QString m_lastFile;
    QSet<QString> m_modifiedFiles;
    QSet<QString> m_savedFiles;

    void loadState();
    void updateProgress();
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
