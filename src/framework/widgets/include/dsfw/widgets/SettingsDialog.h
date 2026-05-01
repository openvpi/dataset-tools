#pragma once

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QDialog>

class QTabWidget;
class QDialogButtonBox;

namespace dstools {

class AppSettings;

}

namespace dsfw::widgets {

class PropertyEditor;

class DSFW_WIDGETS_API SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(dstools::AppSettings *settings, QWidget *parent = nullptr);

    PropertyEditor *addPage(const QString &title);

    int exec() override;

private:
    void applyChanges();

    dstools::AppSettings *m_settings;
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    QList<PropertyEditor *> m_pages;
};

} // namespace dsfw::widgets
