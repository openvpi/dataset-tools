#include <dsfw/widgets/PropertyEditor.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVariant>

namespace dsfw::widgets {

PropertyEditor::PropertyEditor(QWidget *parent) : QWidget(parent) {
    m_layout = new QFormLayout(this);
    m_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
}

void PropertyEditor::addStringProperty(const QString &key, const QString &label,
                                        const QString &defaultValue) {
    auto *widget = new QLineEdit(defaultValue, this);
    m_layout->addRow(label, widget);
    m_widgets[key] = widget;
    m_keyTypes[key] = QStringLiteral("string");
    connect(widget, &QLineEdit::textChanged, this, [this, key](const QString &text) {
        emit propertyChanged(key, text);
    });
}

void PropertyEditor::addIntProperty(const QString &key, const QString &label,
                                     int defaultValue, int min, int max) {
    auto *widget = new QSpinBox(this);
    widget->setRange(min, max);
    widget->setValue(defaultValue);
    m_layout->addRow(label, widget);
    m_widgets[key] = widget;
    m_keyTypes[key] = QStringLiteral("int");
    connect(widget, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this, key](int val) { emit propertyChanged(key, val); });
}

void PropertyEditor::addDoubleProperty(const QString &key, const QString &label,
                                        double defaultValue, double min, double max,
                                        int decimals) {
    auto *widget = new QDoubleSpinBox(this);
    widget->setRange(min, max);
    widget->setDecimals(decimals);
    widget->setValue(defaultValue);
    m_layout->addRow(label, widget);
    m_widgets[key] = widget;
    m_keyTypes[key] = QStringLiteral("double");
    connect(widget, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this, key](double val) { emit propertyChanged(key, val); });
}

void PropertyEditor::addBoolProperty(const QString &key, const QString &label,
                                      bool defaultValue) {
    auto *widget = new QCheckBox(this);
    widget->setChecked(defaultValue);
    m_layout->addRow(label, widget);
    m_widgets[key] = widget;
    m_keyTypes[key] = QStringLiteral("bool");
    connect(widget, &QCheckBox::toggled, this, [this, key](bool val) {
        emit propertyChanged(key, val);
    });
}

void PropertyEditor::addChoiceProperty(const QString &key, const QString &label,
                                        const QStringList &choices, int defaultIndex) {
    auto *widget = new QComboBox(this);
    widget->addItems(choices);
    if (defaultIndex >= 0 && defaultIndex < choices.size())
        widget->setCurrentIndex(defaultIndex);
    m_layout->addRow(label, widget);
    m_widgets[key] = widget;
    m_keyTypes[key] = QStringLiteral("choice");
    connect(widget, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, key](int idx) { emit propertyChanged(key, idx); });
}

QString PropertyEditor::getString(const QString &key) const {
    auto *w = qobject_cast<QLineEdit *>(m_widgets.value(key));
    return w ? w->text() : QString();
}

int PropertyEditor::getInt(const QString &key) const {
    auto *w = qobject_cast<QSpinBox *>(m_widgets.value(key));
    return w ? w->value() : 0;
}

double PropertyEditor::getDouble(const QString &key) const {
    auto *w = qobject_cast<QDoubleSpinBox *>(m_widgets.value(key));
    return w ? w->value() : 0.0;
}

bool PropertyEditor::getBool(const QString &key) const {
    auto *w = qobject_cast<QCheckBox *>(m_widgets.value(key));
    return w ? w->isChecked() : false;
}

int PropertyEditor::getChoice(const QString &key) const {
    auto *w = qobject_cast<QComboBox *>(m_widgets.value(key));
    return w ? w->currentIndex() : -1;
}

void PropertyEditor::setValue(const QString &key, const QVariant &value) {
    if (!m_widgets.contains(key)) return;

    QWidget *w = m_widgets[key];
    if (auto *le = qobject_cast<QLineEdit *>(w)) {
        le->setText(value.toString());
    } else if (auto *sb = qobject_cast<QSpinBox *>(w)) {
        sb->setValue(value.toInt());
    } else if (auto *dsb = qobject_cast<QDoubleSpinBox *>(w)) {
        dsb->setValue(value.toDouble());
    } else if (auto *cb = qobject_cast<QCheckBox *>(w)) {
        cb->setChecked(value.toBool());
    } else if (auto *cmb = qobject_cast<QComboBox *>(w)) {
        cmb->setCurrentIndex(value.toInt());
    }
}

QVariant PropertyEditor::value(const QString &key) const {
    if (!m_widgets.contains(key)) return QVariant();

    QWidget *w = m_widgets[key];
    if (auto *le = qobject_cast<QLineEdit *>(w))
        return le->text();
    if (auto *sb = qobject_cast<QSpinBox *>(w))
        return sb->value();
    if (auto *dsb = qobject_cast<QDoubleSpinBox *>(w))
        return dsb->value();
    if (auto *cb = qobject_cast<QCheckBox *>(w))
        return cb->isChecked();
    if (auto *cmb = qobject_cast<QComboBox *>(w))
        return cmb->currentIndex();
    return QVariant();
}

QStringList PropertyEditor::keys() const {
    return m_widgets.keys();
}

} // namespace dsfw::widgets
