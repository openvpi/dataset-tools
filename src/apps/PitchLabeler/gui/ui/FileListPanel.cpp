#include "FileListPanel.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

namespace dstools {
namespace pitchlabeler {
namespace ui {

// Style constants
static const QString StyleUnsaved   = "color: #9898A8;";                                         // dim gray — not yet touched
static const QString StyleModified  = "color: #FFB366; font-weight: bold;";                      // orange — modified
static const QString StyleSaved     = "color: #4ADE80;";                                         // green — saved

FileListPanel::FileListPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listWidget = new QListWidget();
    layout->addWidget(m_listWidget, 1);

    // Progress tracker at the bottom
    m_progressTracker = new dstools::widgets::FileProgressTracker(
        dstools::widgets::FileProgressTracker::LabelOnly, this);
    m_progressTracker->setFormat(QStringLiteral("\u6807\u6ce8\u8fdb\u5ea6: %1 / %2 (%3%)"));
    m_progressTracker->setEmptyText(QStringLiteral("\u65e0\u6587\u4ef6"));
    layout->addWidget(m_progressTracker);

    // Use currentRowChanged for both click and keyboard navigation
    connect(m_listWidget, &QListWidget::currentRowChanged, this, &FileListPanel::onCurrentRowChanged);
}

FileListPanel::~FileListPanel() = default;

void FileListPanel::setDirectory(const QString &path) {
    m_listWidget->clear();
    m_modifiedFiles.clear();
    m_directory = path;

    if (path.isEmpty()) {
        m_progressTracker->setProgress(m_savedFiles.size(), m_listWidget->count());
        return;
    }

    loadState();

    QDir dir(path);
    QStringList filters{"*.ds"};
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    for (const QFileInfo &fi : files) {
        auto *item = new QListWidgetItem(fi.fileName(), m_listWidget);
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        updateItemStyle(item, fi.absoluteFilePath());
    }

    m_progressTracker->setProgress(m_savedFiles.size(), m_listWidget->count());

    // Restore last selected file
    bool restored = false;
    if (!m_lastFile.isEmpty()) {
        for (int i = 0; i < m_listWidget->count(); ++i) {
            auto *item = m_listWidget->item(i);
            if (item && QFileInfo(item->data(Qt::UserRole).toString()).fileName() == m_lastFile) {
                m_listWidget->setCurrentItem(item);
                restored = true;
                break;
            }
        }
    }

    // Select first if nothing selected — triggers currentRowChanged → fileSelected
    if (!restored && m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void FileListPanel::clear() {
    m_directory.clear();
    m_lastFile.clear();
    m_modifiedFiles.clear();
    m_savedFiles.clear();
    m_listWidget->clear();
    m_progressTracker->setProgress(m_savedFiles.size(), m_listWidget->count());
}

void FileListPanel::populateList() {
    // Re-style all items
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        if (item) {
            updateItemStyle(item, item->data(Qt::UserRole).toString());
        }
    }
}

void FileListPanel::onItemClicked(QListWidgetItem *item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        m_lastFile = QFileInfo(path).fileName();
        emit fileSelected(path);
    }
}

void FileListPanel::onCurrentRowChanged(int row) {
    if (row < 0) return;
    auto *item = m_listWidget->item(row);
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (!path.isEmpty()) {
        m_lastFile = QFileInfo(path).fileName();
        emit fileSelected(path);
    }
}

void FileListPanel::selectNextFile() {
    int row = m_listWidget->currentRow();
    if (row < m_listWidget->count() - 1) {
        m_listWidget->setCurrentRow(row + 1);
    }
}

void FileListPanel::selectPrevFile() {
    int row = m_listWidget->currentRow();
    if (row > 0) {
        m_listWidget->setCurrentRow(row - 1);
    }
}

QString FileListPanel::currentFilePath() const {
    auto *item = m_listWidget->currentItem();
    if (item) {
        return item->data(Qt::UserRole).toString();
    }
    return QString();
}

void FileListPanel::setFileModified(const QString &path, bool modified) {
    if (modified) {
        m_modifiedFiles.insert(path);
    } else {
        m_modifiedFiles.remove(path);
    }

    // Find and restyle the item
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == path) {
            updateItemStyle(item, path);
            break;
        }
    }
}

void FileListPanel::setFileSaved(const QString &path) {
    m_modifiedFiles.remove(path);
    m_savedFiles.insert(path);

    // Find and restyle the item
    for (int i = 0; i < m_listWidget->count(); ++i) {
        auto *item = m_listWidget->item(i);
        if (item && item->data(Qt::UserRole).toString() == path) {
            updateItemStyle(item, path);
            break;
        }
    }
    m_progressTracker->setProgress(m_savedFiles.size(), m_listWidget->count());
}

int FileListPanel::totalFiles() const {
    return m_listWidget->count();
}

int FileListPanel::savedFiles() const {
    return m_savedFiles.size();
}

void FileListPanel::updateItemStyle(QListWidgetItem *item, const QString &path) {
    if (!item) return;

    QString fileName = QFileInfo(path).fileName();

    if (m_modifiedFiles.contains(path)) {
        // Modified — orange, show asterisk
        item->setText(QStringLiteral("● ") + fileName);
        item->setForeground(QColor("#FFB366"));
        QFont f = item->font();
        f.setBold(true);
        item->setFont(f);
    } else if (m_savedFiles.contains(path)) {
        // Saved — green checkmark
        item->setText(QStringLiteral("✓ ") + fileName);
        item->setForeground(QColor("#4ADE80"));
        QFont f = item->font();
        f.setBold(false);
        item->setFont(f);
    } else {
        // Unsaved/untouched — default
        item->setText(fileName);
        item->setForeground(QColor("#9898A8"));
        QFont f = item->font();
        f.setBold(false);
        item->setFont(f);
    }
}

void FileListPanel::saveState() {
    if (m_directory.isEmpty()) return;

    QString stateFile = m_directory + "/.pitchlabeler_state.json";

    QFile file(stateFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonObject json;
        json["last_file"] = m_lastFile;

        // Save the set of saved files
        QJsonArray savedArr;
        for (const QString &path : m_savedFiles) {
            savedArr.append(QFileInfo(path).fileName());
        }
        json["saved_files"] = savedArr;

        file.write(QJsonDocument(json).toJson());
        file.close();
    }
}

void FileListPanel::loadState() {
    if (m_directory.isEmpty()) return;

    QString stateFile = m_directory + "/.pitchlabeler_state.json";

    QFile file(stateFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
        file.close();
        m_lastFile = json["last_file"].toString();

        // Restore saved files set
        m_savedFiles.clear();
        QJsonArray savedArr = json["saved_files"].toArray();
        for (const QJsonValue &val : savedArr) {
            // Reconstruct full path
            QString fullPath = m_directory + "/" + val.toString();
            if (QFile::exists(fullPath)) {
                m_savedFiles.insert(fullPath);
            }
        }
    }
}

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
