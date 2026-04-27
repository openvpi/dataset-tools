#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QStyledItemDelegate>
#include <functional>

namespace dstools::widgets {

/// A QStyledItemDelegate that colors file items based on a status checker callback.
/// Used with QTreeView + QFileSystemModel to visually indicate completed/edited files.
class DSTOOLS_WIDGETS_API FileStatusDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    /// Callback: given a file path, return true if the file is "completed" (will be shown in alt color).
    using StatusChecker = std::function<bool(const QString &filePath)>;

    explicit FileStatusDelegate(QObject *parent = nullptr);
    ~FileStatusDelegate() override;

    /// Set the checker callback. Called during paint() for each item.
    void setStatusChecker(StatusChecker checker);

    /// Set the color for completed items. Default: Qt::gray.
    void setCompletedColor(const QColor &color);

private:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    StatusChecker m_checker;
    QColor m_completedColor = Qt::gray;
};

} // namespace dstools::widgets
