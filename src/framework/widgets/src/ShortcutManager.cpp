#include <dsfw/widgets/ShortcutEditorWidget.h>
#include <dsfw/widgets/ShortcutManager.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace dsfw::widgets {

    ShortcutManager::ShortcutManager(dstools::AppSettings *settings, QObject *parent)
        : QObject(parent), m_settings(settings) {
    }

    void ShortcutManager::bind(QAction *action, const dstools::SettingsKey<QString> &key, const QString &displayName,
                               const QString &category) {
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
            dstools::SettingsKey<QString> key(b.keyPath, b.defaultValue);
            QString seq = m_settings->get(key);
            b.action->setShortcut(QKeySequence(seq));
        }
    }

    void ShortcutManager::saveAll() {
        for (const auto &b : m_bindings) {
            dstools::SettingsKey<QString> key(b.keyPath, b.defaultValue);
            m_settings->set(key, b.action->shortcut().toString());
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
