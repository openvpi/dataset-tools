#pragma once

#include <dsfw/IPageActions.h>

#include <QWidget>

class QProgressBar;
class MainWidget;

namespace dstools {
class AppSettings;
}

class GameInferPage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit GameInferPage(dstools::AppSettings *settings, QWidget *parent = nullptr);

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;
    bool supportsDragDrop() const override;
    void handleDragEnter(QDragEnterEvent *event) override;
    void handleDrop(QDropEvent *event) override;

private:
    MainWidget *m_mainWidget;
    dstools::AppSettings *m_settings;
};
