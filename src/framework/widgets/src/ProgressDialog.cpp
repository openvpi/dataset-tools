#include <dsfw/widgets/ProgressDialog.h>

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace dsfw::widgets {

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
