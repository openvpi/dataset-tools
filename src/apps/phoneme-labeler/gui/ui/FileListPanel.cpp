#include "FileListPanel.h"

#include <QDir>
#include <QListWidget>

namespace dstools {
namespace phonemelabeler {

FileListPanel::FileListPanel(QWidget *parent)
    : BaseFileListPanel(parent)
{
    setFileFilters({"*.TextGrid", "*.textgrid"});
}

QStringList FileListPanel::textGridFiles() const {
    QStringList files;
    for (int i = 0; i < listWidget()->count(); ++i) {
        auto *item = listWidget()->item(i);
        if (item)
            files.append(item->data(Qt::UserRole).toString());
    }
    return files;
}

} // namespace phonemelabeler
} // namespace dstools
