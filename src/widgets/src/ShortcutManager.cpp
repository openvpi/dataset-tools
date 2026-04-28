#include <dstools/ShortcutManager.h>
#include <dstools/ShortcutEditorWidget.h>

#include <QAction>

namespace dstools::widgets {

ShortcutManager::ShortcutManager(dstools::AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {
}

void ShortcutManager::bind(QAction *action, const SettingsKey<QString> &key,
                           const QString &displayName, const QString &category) {
    m_bindings.append({action, key.path, key.defaultValue, displayName, category});
}

void ShortcutManager::applyAll() {
    for (const auto &b : m_bindings) {
        SettingsKey<QString> key(b.keyPath, b.defaultValue);
        b.action->setShortcut(m_settings->shortcut(key));
    }
}

void ShortcutManager::saveAll() {
    for (const auto &b : m_bindings) {
        SettingsKey<QString> key(b.keyPath, b.defaultValue);
        m_settings->setShortcut(key, b.action->shortcut());
    }
}

void ShortcutManager::showEditor(QWidget *parent) {
    std::vector<ShortcutEntry> entries;
    entries.reserve(m_bindings.size());
    for (const auto &b : m_bindings) {
        entries.push_back({b.displayName, b.category, b.keyPath, b.defaultValue});
    }
    ShortcutEditorWidget::showDialog(m_settings, entries, parent);
    applyAll();
}

} // namespace dstools::widgets
