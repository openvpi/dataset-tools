#pragma once

#include <QWidget>

#include <dsfw/IPageLifecycle.h>

class QLabel;
class QPushButton;
class QTextEdit;

class CounterPage : public QWidget, public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageLifecycle)

public:
    explicit CounterPage(QWidget *parent = nullptr);

    // IPageLifecycle
    void onActivated() override;
    void onDeactivated() override;

private:
    int m_count = 0;
    QLabel *m_label = nullptr;
    QTextEdit *m_log = nullptr;
};
