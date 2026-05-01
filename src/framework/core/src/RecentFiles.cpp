#include <dsfw/RecentFiles.h>

namespace dstools {

RecentFiles::RecentFiles(const QString &settingsKey, int maxCount, QObject *parent)
    : QObject(parent), m_settingsKey(settingsKey), m_maxCount(maxCount) {
    load();
}

void RecentFiles::addFile(const QString &filePath) {
    m_files.removeAll(filePath);
    m_files.prepend(filePath);
    while (m_files.size() > m_maxCount)
        m_files.removeLast();
    save();
    emit listChanged();
}

void RecentFiles::removeFile(const QString &filePath) {
    if (m_files.removeAll(filePath) > 0) {
        save();
        emit listChanged();
    }
}

void RecentFiles::clear() {
    if (!m_files.isEmpty()) {
        m_files.clear();
        save();
        emit listChanged();
    }
}

QStringList RecentFiles::files() const { return m_files; }
int RecentFiles::maxCount() const { return m_maxCount; }

void RecentFiles::setMaxCount(int count) {
    m_maxCount = count;
    while (m_files.size() > m_maxCount)
        m_files.removeLast();
    save();
}

void RecentFiles::load() {
    QSettings settings;
    m_files = settings.value(m_settingsKey).toStringList();
    while (m_files.size() > m_maxCount)
        m_files.removeLast();
}

void RecentFiles::save() {
    QSettings settings;
    settings.setValue(m_settingsKey, m_files);
}

} // namespace dstools
