#pragma once

#include <QWidget>
#include <QStringList>
#include <QListWidget>

namespace dstools {
namespace phonemelabeler {

class FileListPanel : public QWidget {
    Q_OBJECT

public:
    explicit FileListPanel(QWidget *parent = nullptr);
    ~FileListPanel() override = default;

    void setDirectory(const QString &dir);
    void setCurrentFile(const QString &path);

    [[nodiscard]] QStringList textGridFiles() const { return m_files; }

signals:
    void fileSelected(const QString &path);

private:
    void refreshFileList();

    QString m_directory;
    QStringList m_files;
    QListWidget *m_listWidget = nullptr;
};

} // namespace phonemelabeler
} // namespace dstools
