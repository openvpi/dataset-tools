#pragma once
/// @file SettingsDialog.h
/// @brief Tabbed settings dialog built from PropertyEditor pages.

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QDialog>

class QTabWidget;
class QDialogButtonBox;

namespace dstools {

class AppSettings;

}

namespace dsfw::widgets {

class PropertyEditor;

/// @brief Tabbed settings dialog backed by AppSettings and PropertyEditor pages.
class DSFW_WIDGETS_API SettingsDialog : public QDialog {
    Q_OBJECT
public:
    /// @brief Construct a SettingsDialog.
    /// @param settings Application settings instance.
    /// @param parent Parent widget.
    explicit SettingsDialog(dstools::AppSettings *settings, QWidget *parent = nullptr);

    /// @brief Add a settings page tab.
    /// @param title Tab title.
    /// @return Pointer to the PropertyEditor for adding properties.
    PropertyEditor *addPage(const QString &title);

    /// @brief Execute the dialog modally, applying changes on accept.
    /// @return Dialog result code.
    int exec() override;

private:
    void applyChanges();

    dstools::AppSettings *m_settings;
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    QList<PropertyEditor *> m_pages;
};

} // namespace dsfw::widgets
