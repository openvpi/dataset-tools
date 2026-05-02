#pragma once

#include <QWidget>

#include <dsfw/IPageLifecycle.h>

class QLabel;
class QTextEdit;

class HelloPage : public QWidget, public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageLifecycle)

public:
    explicit HelloPage(QWidget *parent = nullptr);

    // IPageLifecycle
    void onActivated() override;
    void onDeactivated() override;

private:
    QTextEdit *m_log = nullptr;
};
