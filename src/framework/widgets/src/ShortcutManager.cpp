#include <dsfw/widgets/ShortcutManager.h>
#include <dsfw/widgets/ShortcutEditorWidget.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace dsfw::widgets {

ShortcutManager::ShortcutManager(dstools::AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings) {}

void ShortcutManager::bind(QAction *action, const SettingsKey<QString> &key,
                            const QString &displayName, const QString &category) {
    Binding b;
    b.action = action;
    b.keyPath = key.path;
    b.defaultValue = key.defaultValue;
    b.displayName = displayName;
    b.category = category;
    m_bindings.append(b);
}

void ShortcutManager::applyAll() {
    for (const auto &b : m_bindings) {
        QString seq = m_settings->value(b.keyPath, b.defaultValue);
        b.action->setShortcut(QKeySequence(seq));
    }
}

void ShortcutManager::saveAll() {
    for (const auto &b : m_bindings) {
        m_settings->setValue(b.keyPath, b.action->shortcut().toString());
    }
}

void ShortcutManager::showEditor(QWidget *parent) {
    std::vector<ShortcutEntry> entries;
    for (const auto &b : m_bindings) {
        ShortcutEntry e;
        e.displayName = b.displayName;
        e.category = b.category;
        e.settingsKeyPath = b.keyPath;
        e.defaultSequence = b.defaultValue;
        entries.push_back(e);
    }
    ShortcutEditorWidget::showDialog(m_settings, entries, parent);
    applyAll();
}

void ShortcutManager::setEnabled(bool enabled) {
    for (const auto &b : m_bindings) {
        b.action->setEnabled(enabled);
    }
}

} // namespace dsfw::widgets
