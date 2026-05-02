#include "HelloPage.h"

#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

HelloPage::HelloPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *title = new QLabel(QStringLiteral("Hello, dsfw!"));
    title->setAlignment(Qt::AlignCenter);
    QFont f = title->font();
    f.setPointSize(20);
    title->setFont(f);
    layout->addWidget(title);

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText(QStringLiteral("Lifecycle events appear here..."));
    layout->addWidget(m_log);
}

void HelloPage::onActivated() {
    m_log->append(QStringLiteral("▶ onActivated"));
}

void HelloPage::onDeactivated() {
    m_log->append(QStringLiteral("■ onDeactivated"));
}
