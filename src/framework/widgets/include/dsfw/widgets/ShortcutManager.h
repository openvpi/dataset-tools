#pragma once
/// @file ShortcutManager.h
/// @brief Manages keyboard shortcut bindings between QActions and persistent settings.

#include <dsfw/widgets/WidgetsGlobal.h>
#include <dsfw/AppSettings.h>

#include <QObject>
#include <QList>

class QAction;
class QWidget;

namespace dsfw::widgets {

/// @brief Manages keyboard shortcut bindings with persistent storage.
class DSFW_WIDGETS_API ShortcutManager : public QObject {
    Q_OBJECT
public:
    /// @brief Construct a ShortcutManager.
    /// @param settings Application settings for shortcut persistence.
    /// @param parent Parent QObject.
    explicit ShortcutManager(dstools::AppSettings *settings, QObject *parent = nullptr);

    /// @brief Bind a QAction to a settings key for shortcut customization.
    /// @param action Action to bind.
    /// @param key Settings key storing the shortcut sequence.
    /// @param displayName Human-readable name shown in the editor.
    /// @param category Category grouping for the editor.
    void bind(QAction *action, const dstools::SettingsKey<QString> &key,
              const QString &displayName, const QString &category);

    /// @brief Apply all saved shortcuts to their bound actions.
    void applyAll();

    /// @brief Save all current shortcut values to settings.
    void saveAll();

    /// @brief Show the shortcut editor dialog.
    /// @param parent Parent widget for the dialog.
    void showEditor(QWidget *parent);

    /// @brief Enable or disable all managed shortcuts.
    /// @param enabled True to enable, false to disable.
    void setEnabled(bool enabled);

    /// @brief Update tooltips for all bound actions to include their shortcut key.
    ///
    /// Appends " (shortcut)" to each action's tooltip based on its current shortcut.
    /// Call after applyAll() to annotate tool buttons with shortcut hints.
    void updateTooltips();

private:
    struct Binding {
        QAction *action;
        const char *keyPath;
        QString defaultValue;
        QString displayName;
        QString category;
    };
    QList<Binding> m_bindings;
    dstools::AppSettings *m_settings;
};

} // namespace dsfw::widgets
