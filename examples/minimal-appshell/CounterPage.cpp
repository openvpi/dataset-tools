#include "CounterPage.h"

#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

CounterPage::CounterPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    m_label = new QLabel(QStringLiteral("Clicks: 0"));
    m_label->setAlignment(Qt::AlignCenter);
    QFont f = m_label->font();
    f.setPointSize(18);
    m_label->setFont(f);
    layout->addWidget(m_label);

    auto *button = new QPushButton(QStringLiteral("Click me"));
    layout->addWidget(button);

    connect(button, &QPushButton::clicked, this, [this]() {
        ++m_count;
        m_label->setText(QStringLiteral("Clicks: %1").arg(m_count));
    });

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    m_log->setPlaceholderText(QStringLiteral("Lifecycle events appear here..."));
    layout->addWidget(m_log);
}

void CounterPage::onActivated() {
    m_log->append(QStringLiteral("▶ onActivated"));
}

void CounterPage::onDeactivated() {
    m_log->append(QStringLiteral("■ onDeactivated"));
}
