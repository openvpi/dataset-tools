#pragma once
#include <QMainWindow>
#include <dsfw/AppSettings.h>

class PipelinePage;

class PipelineWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PipelineWindow(QWidget *parent = nullptr);
    ~PipelineWindow() override = default;

private:
    PipelinePage *m_page;
    dstools::AppSettings m_settings;
};
