#pragma once

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

class DSFW_WIDGETS_API PropertyEditor : public QWidget {
    Q_OBJECT
public:
    explicit PropertyEditor(QWidget *parent = nullptr);

    void addStringProperty(const QString &key, const QString &label,
                           const QString &defaultValue = {});
    void addIntProperty(const QString &key, const QString &label,
                        int defaultValue = 0, int min = INT_MIN, int max = INT_MAX);
    void addDoubleProperty(const QString &key, const QString &label,
                           double defaultValue = 0.0, double min = -1e9, double max = 1e9,
                           int decimals = 2);
    void addBoolProperty(const QString &key, const QString &label, bool defaultValue = false);
    void addChoiceProperty(const QString &key, const QString &label,
                           const QStringList &choices, int defaultIndex = 0);

    QString getString(const QString &key) const;
    int getInt(const QString &key) const;
    double getDouble(const QString &key) const;
    bool getBool(const QString &key) const;
    int getChoice(const QString &key) const;

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key) const;

    QStringList keys() const;

signals:
    void propertyChanged(const QString &key, const QVariant &value);

private:
    QFormLayout *m_layout;
    QHash<QString, QWidget *> m_widgets;
    QHash<QString, QString> m_keyTypes;
};

} // namespace dsfw::widgets
