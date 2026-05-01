/// @file RecentFilesManager.cpp
/// @brief Implementation of RecentFilesManager.

#include <dsfw/RecentFilesManager.h>

#include <dsfw/AppSettings.h>

#include <QAction>
#include <QFileInfo>
#include <QMenu>

namespace {
const dstools::SettingsKey<QString> kRecentFiles("General/recentFiles", QString());
}

namespace dsfw {

RecentFilesManager::RecentFilesManager(dstools::AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {
    load();
}

void RecentFilesManager::addFile(const QString &filePath) {
    if (filePath.isEmpty())
        return;

    m_files.removeAll(filePath);
    m_files.prepend(filePath);

    while (m_files.size() > m_maxCount)
        m_files.removeLast();

    save();
    rebuildMenus();
    emit listChanged();
}

QStringList RecentFilesManager::fileList() const {
    return m_files;
}

void RecentFilesManager::clear() {
    if (m_files.isEmpty())
        return;

    m_files.clear();
    save();
    rebuildMenus();
    emit listChanged();
}

void RecentFilesManager::setMaxCount(int max) {
    if (max < 1)
        max = 1;
    m_maxCount = max;

    if (m_files.size() > m_maxCount) {
        while (m_files.size() > m_maxCount)
            m_files.removeLast();
        save();
        rebuildMenus();
        emit listChanged();
    }
}

int RecentFilesManager::maxCount() const {
    return m_maxCount;
}

QMenu *RecentFilesManager::createMenu(const QString &title, QWidget *parent) {
    auto *menu = new QMenu(title, parent);
    m_menus.append(QPointer<QMenu>(menu));

    // Populate initially
    if (m_files.isEmpty()) {
        auto *empty = menu->addAction(tr("(No Recent Files)"));
        empty->setEnabled(false);
    } else {
        for (const auto &path : m_files) {
            auto *action = menu->addAction(QFileInfo(path).fileName());
            action->setToolTip(path);
            action->setStatusTip(path);
            action->setData(path);
            connect(action, &QAction::triggered, this, [this, path]() {
                emit fileTriggered(path);
            });
        }
        menu->addSeparator();
        auto *clearAction = menu->addAction(tr("Clear Recent Files"));
        connect(clearAction, &QAction::triggered, this, &RecentFilesManager::clear);
    }

    return menu;
}

void RecentFilesManager::load() {
    const QString raw = m_settings->get(kRecentFiles);
    if (raw.isEmpty()) {
        m_files.clear();
        return;
    }
    m_files = raw.split(QLatin1Char('|'), Qt::SkipEmptyParts);

    while (m_files.size() > m_maxCount)
        m_files.removeLast();
}

void RecentFilesManager::save() {
    m_settings->set(kRecentFiles, m_files.join(QLatin1Char('|')));
}

void RecentFilesManager::rebuildMenus() {
    // Remove dead pointers
    m_menus.removeAll(QPointer<QMenu>(nullptr));

    for (auto &menuPtr : m_menus) {
        if (!menuPtr)
            continue;

        QMenu *menu = menuPtr.data();
        menu->clear();

        if (m_files.isEmpty()) {
            auto *empty = menu->addAction(tr("(No Recent Files)"));
            empty->setEnabled(false);
        } else {
            for (const auto &path : m_files) {
                auto *action = menu->addAction(QFileInfo(path).fileName());
                action->setToolTip(path);
                action->setStatusTip(path);
                action->setData(path);
                connect(action, &QAction::triggered, this, [this, path]() {
                    emit fileTriggered(path);
                });
            }
            menu->addSeparator();
            auto *clearAction = menu->addAction(tr("Clear Recent Files"));
            connect(clearAction, &QAction::triggered, this, &RecentFilesManager::clear);
        }
    }
}

} // namespace dsfw
