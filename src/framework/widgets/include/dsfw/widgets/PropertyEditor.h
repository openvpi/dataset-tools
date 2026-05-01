#pragma once
/// @file PropertyEditor.h
/// @brief Dynamic form-based property editor supporting multiple value types.

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QWidget>
#include <QHash>

class QFormLayout;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;

namespace dsfw::widgets {

/// @brief Dynamic form-based property editor with typed accessors.
class DSFW_WIDGETS_API PropertyEditor : public QWidget {
    Q_OBJECT
public:
    /// @brief Construct a PropertyEditor.
    /// @param parent Parent widget.
    explicit PropertyEditor(QWidget *parent = nullptr);

    /// @brief Add a string property row.
    /// @param key Unique property key.
    /// @param label Display label.
    /// @param defaultValue Default string value.
    void addStringProperty(const QString &key, const QString &label,
                           const QString &defaultValue = {});

    /// @brief Add an integer property row.
    /// @param key Unique property key.
    /// @param label Display label.
    /// @param defaultValue Default integer value.
    /// @param min Minimum allowed value.
    /// @param max Maximum allowed value.
    void addIntProperty(const QString &key, const QString &label,
                        int defaultValue = 0, int min = INT_MIN, int max = INT_MAX);

    /// @brief Add a double property row.
    /// @param key Unique property key.
    /// @param label Display label.
    /// @param defaultValue Default double value.
    /// @param min Minimum allowed value.
    /// @param max Maximum allowed value.
    /// @param decimals Number of decimal places.
    void addDoubleProperty(const QString &key, const QString &label,
                           double defaultValue = 0.0, double min = -1e9, double max = 1e9,
                           int decimals = 2);

    /// @brief Add a boolean property row.
    /// @param key Unique property key.
    /// @param label Display label.
    /// @param defaultValue Default boolean value.
    void addBoolProperty(const QString &key, const QString &label, bool defaultValue = false);

    /// @brief Add a choice (combo box) property row.
    /// @param key Unique property key.
    /// @param label Display label.
    /// @param choices List of choice strings.
    /// @param defaultIndex Default selected index.
    void addChoiceProperty(const QString &key, const QString &label,
                           const QStringList &choices, int defaultIndex = 0);

    /// @brief Get a string property value.
    /// @param key Property key.
    /// @return String value.
    QString getString(const QString &key) const;

    /// @brief Get an integer property value.
    /// @param key Property key.
    /// @return Integer value.
    int getInt(const QString &key) const;

    /// @brief Get a double property value.
    /// @param key Property key.
    /// @return Double value.
    double getDouble(const QString &key) const;

    /// @brief Get a boolean property value.
    /// @param key Property key.
    /// @return Boolean value.
    bool getBool(const QString &key) const;

    /// @brief Get a choice property index.
    /// @param key Property key.
    /// @return Selected index.
    int getChoice(const QString &key) const;

    /// @brief Set a property value by key.
    /// @param key Property key.
    /// @param value Value to set.
    void setValue(const QString &key, const QVariant &value);

    /// @brief Get a property value by key.
    /// @param key Property key.
    /// @return Property value.
    QVariant value(const QString &key) const;

    /// @brief Get all property keys.
    /// @return List of keys.
    QStringList keys() const;

signals:
    /// @brief Emitted when any property value changes.
    /// @param key Changed property key.
    /// @param value New property value.
    void propertyChanged(const QString &key, const QVariant &value);

private:
    QFormLayout *m_layout;
    QHash<QString, QWidget *> m_widgets;
    QHash<QString, QString> m_keyTypes;
};

} // namespace dsfw::widgets
