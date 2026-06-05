#include <dsfw/widgets/FilePathSelector.h>
#include <dsfw/FileDialogHelper.h>
#include <dsfw/widgets/RecentPathStore.h>

#include <QDir>
#include <QFileInfo>

namespace dsfw::widgets {

FilePathSelector::FilePathSelector(const Config &config, QWidget *parent)
    : QObject(parent), m_config(config) {
}

QString FilePathSelector::selectedPath() const {
    return m_selectedPath;
}

QStringList FilePathSelector::selectedPaths() const {
    return m_selectedPaths;
}

QString FilePathSelector::exec() {
    const QString recentPath = RecentPathStore::load(m_config.settingsKey);

    FileDialogHelper::Options opts;
    opts.parent = qobject_cast<QWidget *>(parent());
    opts.title = m_config.title;
    opts.defaultDir = recentPath.isEmpty() ? m_config.initialPath : recentPath;
    opts.nameFilters = m_config.nameFilters;
    opts.defaultSuffix = m_config.defaultSuffix;

    switch (m_config.mode) {
        case Mode::OpenFile: {
            m_selectedPath = FileDialogHelper::getOpenFileName(opts);
            m_selectedPaths.clear();
            if (!m_selectedPath.isEmpty())
                m_selectedPaths.append(m_selectedPath);
            break;
        }
        case Mode::OpenFiles: {
            m_selectedPaths = FileDialogHelper::getOpenFileNames(opts);
            m_selectedPath = m_selectedPaths.isEmpty() ? QString() : m_selectedPaths.first();
            break;
        }
        case Mode::SaveFile: {
            m_selectedPath = FileDialogHelper::getSaveFileName(opts);
            m_selectedPaths.clear();
            if (!m_selectedPath.isEmpty())
                m_selectedPaths.append(m_selectedPath);
            break;
        }
        case Mode::OpenDirectory: {
            opts.defaultDir = recentPath.isEmpty() ? m_config.initialPath : recentPath;
            m_selectedPath = FileDialogHelper::getExistingDirectory(opts);
            m_selectedPaths.clear();
            if (!m_selectedPath.isEmpty())
                m_selectedPaths.append(m_selectedPath);
            break;
        }
    }

    if (!m_selectedPath.isEmpty()) {
        RecentPathStore::save(m_config.settingsKey, m_selectedPath);
        emit pathSelected(m_selectedPath);
        if (!m_selectedPaths.isEmpty())
            emit pathsSelected(m_selectedPaths);
    }

    return m_selectedPath;
}

} // namespace dsfw::widgets
