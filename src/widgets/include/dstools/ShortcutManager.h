#pragma once

#include <dstools/WidgetsGlobal.h>
#include <dstools/AppSettings.h>

#include <QObject>
#include <QList>

class QAction;
class QWidget;

namespace dstools::widgets {

/// Centralized QAction ↔ settings key binding manager.
/// Eliminates manual shortcut re-apply loops after editing.
class DSTOOLS_WIDGETS_API ShortcutManager : public QObject {
    Q_OBJECT
public:
    explicit ShortcutManager(dstools::AppSettings *settings, QObject *parent = nullptr);

    /// Register an action with its settings key, display name and category.
    void bind(QAction *action, const SettingsKey<QString> &key,
              const QString &displayName, const QString &category);

    /// Read all bound shortcuts from settings and apply to actions.
    void applyAll();

    /// Save all bound action shortcuts to settings.
    void saveAll();

    /// Show the shortcut editor dialog. Calls applyAll() after dialog closes.
    void showEditor(QWidget *parent);

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

} // namespace dstools::widgets
