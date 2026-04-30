#pragma once

#include <IPageActions.h>

#include <QWidget>

class QTabWidget;
class SlicerPage;
class LyricFAPage;
class HubertFAPage;

namespace dstools {
class AppSettings;
}

class PipelinePage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit PipelinePage(dstools::AppSettings *settings, QWidget *parent = nullptr);

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QWidget *createStatusBarContent(QWidget *parent) override;
    QString windowTitle() const override;

private:
    QTabWidget *m_tabWidget;
    SlicerPage *m_slicerPage;
    LyricFAPage *m_lyricFAPage;
    HubertFAPage *m_hubertFAPage;

    dstools::AppSettings *m_settings;
};
