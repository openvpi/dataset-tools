#include "FileListPanel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>

#include <dstools/FileProgressTracker.h>

namespace dstools {
    namespace pitchlabeler {
        namespace ui {

            FileListPanel::FileListPanel(QWidget *parent) : BaseFileListPanel(parent) {
                setFileFilters({"*.ds"});
                setShowProgress(true);
                progressTracker()->setFormat(QStringLiteral("已标注 %1 / %2 (%3%)"));
                progressTracker()->setEmptyText(QStringLiteral("未加载文件"));
            }

            FileListPanel::~FileListPanel() = default;

            void FileListPanel::setDirectory(const QString &path) {
                m_modifiedFiles.clear();

                if (path.isEmpty()) {
                    BaseFileListPanel::setDirectory(path);
                    return;
                }

                // Load persisted state before populating (needs path for state file)
                // Temporarily store path so loadState can use it via a local variable
                m_savedFiles.clear();
                // We need directory set to load state, but base::setDirectory also refreshes.
                // So: set directory in base (which refreshes + calls styleItem with empty saved set),
                // then load state, re-style, and restore selection.
                BaseFileListPanel::setDirectory(path); // sets m_directory, refreshes list

                // Now load persisted state
                loadState(); // uses directory() which is now set

                // Re-style items with loaded state
                for (int i = 0; i < listWidget()->count(); ++i) {
                    auto *item = listWidget()->item(i);
                    if (item)
                        styleItem(item, item->data(Qt::UserRole).toString());
                }

                // Restore last selected file
                bool restored = false;
                if (!m_lastFile.isEmpty()) {
                    for (int i = 0; i < listWidget()->count(); ++i) {
                        auto *item = listWidget()->item(i);
                        if (item && QFileInfo(item->data(Qt::UserRole).toString()).fileName() == m_lastFile) {
                            listWidget()->setCurrentItem(item);
                            restored = true;
                            break;
                        }
                    }
                }

                // Select first if nothing selected — triggers currentRowChanged → fileSelected
                if (!restored && listWidget()->count() > 0) {
                    listWidget()->setCurrentRow(0);
                }

                updateProgress();
            }

            void FileListPanel::clear() {
                m_lastFile.clear();
                m_modifiedFiles.clear();
                m_savedFiles.clear();
                BaseFileListPanel::setDirectory(QString());
                updateProgress();
            }

            void FileListPanel::setFileModified(const QString &path, bool modified) {
                if (modified) {
                    m_modifiedFiles.insert(path);
                } else {
                    m_modifiedFiles.remove(path);
                }

                // Find and restyle the item
                for (int i = 0; i < listWidget()->count(); ++i) {
                    auto *item = listWidget()->item(i);
                    if (item && item->data(Qt::UserRole).toString() == path) {
                        styleItem(item, path);
                        break;
                    }
                }
            }

            void FileListPanel::setFileSaved(const QString &path) {
                m_modifiedFiles.remove(path);
                m_savedFiles.insert(path);

                // Find and restyle the item
                for (int i = 0; i < listWidget()->count(); ++i) {
                    auto *item = listWidget()->item(i);
                    if (item && item->data(Qt::UserRole).toString() == path) {
                        styleItem(item, path);
                        break;
                    }
                }

                updateProgress();
            }

            int FileListPanel::totalFiles() const {
                return fileCount();
            }

            int FileListPanel::savedFiles() const {
                return static_cast<int>(m_savedFiles.size());
            }

            void FileListPanel::styleItem(QListWidgetItem *item, const QString &filePath) {
                if (!item)
                    return;

                QString fileName = QFileInfo(filePath).fileName();

                if (m_modifiedFiles.contains(filePath)) {
                    // Modified — orange, show asterisk
                    item->setText(QStringLiteral("● ") + fileName);
                    item->setForeground(QColor("#FFB366"));
                    QFont f = item->font();
                    f.setBold(true);
                    item->setFont(f);
                } else if (m_savedFiles.contains(filePath)) {
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
                if (directory().isEmpty())
                    return;

                // Update m_lastFile from current selection
                auto *current = listWidget()->currentItem();
                if (current)
                    m_lastFile = QFileInfo(current->data(Qt::UserRole).toString()).fileName();

                QString stateFile = directory() + "/.pitchlabeler_state.json";

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
                } else {
                    qWarning() << "PitchLabeler: Failed to save state file:" << stateFile;
                    QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(file.errorString()));
                }
            }

            void FileListPanel::updateProgress() {
                if (progressTracker()) {
                    progressTracker()->setProgress(
                        static_cast<int>(m_savedFiles.size()), fileCount());
                }
            }

            void FileListPanel::loadState() {
                if (directory().isEmpty())
                    return;

                QString stateFile = directory() + "/.pitchlabeler_state.json";

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
                        QString fullPath = directory() + "/" + val.toString();
                        if (QFile::exists(fullPath)) {
                            m_savedFiles.insert(fullPath);
                        }
                    }
                } else if (QFile::exists(stateFile)) {
                    qWarning() << "PitchLabeler: State file exists but cannot be opened:" << stateFile;
                    QMessageBox::warning(this, tr("Error"), tr("Failed to open state file: %1").arg(file.errorString()));
                } else {
                    qDebug() << "PitchLabeler: No saved state file found (first run or file removed):" << stateFile;
                }
            }

        } // namespace ui
    } // namespace pitchlabeler
} // namespace dstools
