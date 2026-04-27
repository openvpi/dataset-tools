#include <dstools/FileStatusDelegate.h>

#include <QFileSystemModel>
#include <QPainter>

namespace dstools::widgets {

FileStatusDelegate::FileStatusDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {
}

FileStatusDelegate::~FileStatusDelegate() = default;

void FileStatusDelegate::setStatusChecker(StatusChecker checker) {
    m_checker = std::move(checker);
}

void FileStatusDelegate::setCompletedColor(const QColor &color) {
    m_completedColor = color;
}

void FileStatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                               const QModelIndex &index) const {
    const auto *model = dynamic_cast<const QFileSystemModel *>(index.model());
    if (!model) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem modifiedOption(option);

    if (m_checker) {
        const QString filePath = model->filePath(index);
        if (m_checker(filePath)) {
            modifiedOption.palette.setColor(QPalette::Text, m_completedColor);
        }
    }

    QStyledItemDelegate::paint(painter, modifiedOption, index);
}

} // namespace dstools::widgets
