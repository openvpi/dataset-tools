#pragma once

#include <dsfw/widgets/WidgetsGlobal.h>
#include <dsfw/AppSettings.h>

#include <QObject>
#include <QList>

class QAction;
class QWidget;

namespace dsfw::widgets {

class DSFW_WIDGETS_API ShortcutManager : public QObject {
    Q_OBJECT
public:
    explicit ShortcutManager(dstools::AppSettings *settings, QObject *parent = nullptr);

    void bind(QAction *action, const dstools::SettingsKey<QString> &key,
              const QString &displayName, const QString &category);

    void applyAll();
    void saveAll();
    void showEditor(QWidget *parent);
    void setEnabled(bool enabled);

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
