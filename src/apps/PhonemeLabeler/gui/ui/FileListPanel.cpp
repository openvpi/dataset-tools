#include "FileListPanel.h"

#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QSignalBlocker>

namespace dstools {
namespace phonemelabeler {

FileListPanel::FileListPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget);

    connect(m_listWidget, &QListWidget::currentItemChanged,
            [this](QListWidgetItem *current, QListWidgetItem *) {
                if (current) {
                    QString filePath = m_directory + "/" + current->text();
                    emit fileSelected(filePath);
                }
            });
}

void FileListPanel::setDirectory(const QString &dir) {
    m_directory = dir;
    refreshFileList();
}

void FileListPanel::setCurrentFile(const QString &path) {
    QFileInfo fi(path);
    QSignalBlocker blocker(m_listWidget);
    QList<QListWidgetItem *> items = m_listWidget->findItems(fi.fileName(), Qt::MatchExactly);
    if (!items.isEmpty()) {
        m_listWidget->setCurrentItem(items.first());
    }
}

void FileListPanel::refreshFileList() {
    m_listWidget->clear();
    m_files.clear();

    if (m_directory.isEmpty()) return;

    QDir dir(m_directory);
    QStringList filters;
    filters << "*.TextGrid" << "*.textgrid";
    dir.setNameFilters(filters);

    QStringList files = dir.entryList(QDir::Files);
    for (const auto &file : files) {
        m_files.append(file);
        m_listWidget->addItem(file);
    }
}

} // namespace phonemelabeler
} // namespace dstools
