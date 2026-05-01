#include <dsfw/widgets/SettingsDialog.h>
#include <dsfw/widgets/PropertyEditor.h>
#include <dsfw/AppSettings.h>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTabWidget>

namespace dsfw::widgets {

SettingsDialog::SettingsDialog(dstools::AppSettings *settings, QWidget *parent)
    : QDialog(parent), m_settings(settings) {
    setWindowTitle(tr("Settings"));
    resize(500, 400);

    auto *layout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    layout->addWidget(m_tabWidget);

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    layout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::applyChanges);
}

PropertyEditor *SettingsDialog::addPage(const QString &title) {
    auto *page = new PropertyEditor(this);
    m_tabWidget->addTab(page, title);
    m_pages.append(page);
    return page;
}

int SettingsDialog::exec() {
    return QDialog::exec();
}

void SettingsDialog::applyChanges() {
}

} // namespace dsfw::widgets
