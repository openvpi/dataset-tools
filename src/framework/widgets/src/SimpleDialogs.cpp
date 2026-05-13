#include <dsfw/widgets/SettingsDialog.h>
#include <dsfw/widgets/ProgressDialog.h>
#include <dsfw/widgets/PropertyEditor.h>
#include <dsfw/AppSettings.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

namespace dsfw::widgets {

// ─── SettingsDialog ───────────────────────────────────────────────────────────

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

// ─── ProgressDialog ──────────────────────────────────────────────────────────

ProgressDialog::ProgressDialog(const QString &title, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(title.isEmpty() ? tr("Progress") : title);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setMinimumWidth(300);

    auto *layout = new QVBoxLayout(this);

    m_label = new QLabel(this);
    layout->addWidget(m_label);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    layout->addWidget(m_progressBar);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_cancelBtn->setEnabled(false);
    layout->addWidget(m_cancelBtn, 0, Qt::AlignRight);

    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        emit canceled();
    });
}

void ProgressDialog::setRange(int minimum, int maximum) {
    m_progressBar->setRange(minimum, maximum);
}

void ProgressDialog::setValue(int value) {
    m_value = value;
    m_progressBar->setValue(value);
}

int ProgressDialog::value() const {
    return m_value;
}

void ProgressDialog::setLabelText(const QString &text) {
    m_label->setText(text);
}

QString ProgressDialog::labelText() const {
    return m_label->text();
}

void ProgressDialog::setCancelButtonEnabled(bool enabled) {
    m_cancelBtn->setEnabled(enabled);
}

} // namespace dsfw::widgets
