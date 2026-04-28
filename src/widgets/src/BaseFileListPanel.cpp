#include <dstools/BaseFileListPanel.h>
#include <dstools/FileProgressTracker.h>

#include <QDir>
#include <QFileInfo>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QSignalBlocker>

namespace dstools::widgets {

BaseFileListPanel::BaseFileListPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget, 1);

    m_progressTracker = new FileProgressTracker(FileProgressTracker::LabelOnly, this);
    m_progressTracker->setVisible(false);
    layout->addWidget(m_progressTracker);

    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, &BaseFileListPanel::onCurrentRowChanged);
}

BaseFileListPanel::~BaseFileListPanel() = default;

void BaseFileListPanel::setDirectory(const QString &path) {
    m_directory = path;
    refreshFileList();
}

void BaseFileListPanel::setFileFilters(const QStringList &filters) {
    m_filters = filters;
}

void BaseFileListPanel::setShowProgress(bool show) {
    m_showProgress = show;
    if (m_progressTracker)
        m_progressTracker->setVisible(show);
}

QString BaseFileListPanel::currentFile() const {
    auto *item = m_listWidget->currentItem();
    if (item)
        return item->data(Qt::UserRole).toString();
    return {};
}

void BaseFileListPanel::setCurrentFile(const QString &path) {
    QFileInfo fi(path);
    QSignalBlocker blocker(m_listWidget);
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        if (item) {
            QString itemPath = item->data(Qt::UserRole).toString();
            if (QFileInfo(itemPath).fileName() == fi.fileName()) {
                m_listWidget->setCurrentItem(item);
                return;
            }
        }
    }
}

void BaseFileListPanel::selectNextFile() {
    int row = m_listWidget->currentRow();
    if (row < m_listWidget->count() - 1)
        m_listWidget->setCurrentRow(row + 1);
}

void BaseFileListPanel::selectPrevFile() {
    int row = m_listWidget->currentRow();
    if (row > 0)
        m_listWidget->setCurrentRow(row - 1);
}

int BaseFileListPanel::fileCount() const {
    return m_listWidget->count();
}

void BaseFileListPanel::styleItem(QListWidgetItem *item, const QString &filePath) {
    Q_UNUSED(filePath)
    // Default: just filename as text
    item->setText(QFileInfo(item->data(Qt::UserRole).toString()).fileName());
}

void BaseFileListPanel::refreshFileList() {
    m_listWidget->clear();

    if (m_directory.isEmpty()) return;

    QDir dir(m_directory);
    dir.setNameFilters(m_filters);
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);

    for (const QFileInfo &fi : files) {
        auto *item = new QListWidgetItem(fi.fileName(), m_listWidget);
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        styleItem(item, fi.absoluteFilePath());
    }

    emit fileListRefreshed(m_listWidget->count());
}

void BaseFileListPanel::onCurrentRowChanged(int row) {
    if (row < 0) return;
    auto *item = m_listWidget->item(row);
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty())
        emit fileSelected(path);
}

} // namespace dstools::widgets
