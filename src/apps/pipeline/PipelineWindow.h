#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <dstools/Config.h>

class SlicerPage;
class LyricFAPage;
class HubertFAPage;

class PipelineWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PipelineWindow(QWidget *parent = nullptr);
    ~PipelineWindow() override;

private:
    void setupUI();
    void setupMenuBar();

    QTabWidget *m_tabWidget;
    SlicerPage *m_slicerPage;
    LyricFAPage *m_lyricFAPage;
    HubertFAPage *m_hubertFAPage;

    dstools::Config m_config;
};
