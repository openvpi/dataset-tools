#include "PipelineWindow.h"
#include "PipelinePage.h"

#include <QMenuBar>
#include <QStatusBar>

static constexpr int kDefaultWidth = 1200;
static constexpr int kDefaultHeight = 800;

PipelineWindow::PipelineWindow(QWidget *parent)
    : QMainWindow(parent), m_settings("DatasetPipeline") {
    m_page = new PipelinePage(&m_settings, this);

    setWindowTitle(m_page->windowTitle());
    resize(kDefaultWidth, kDefaultHeight);
    setCentralWidget(m_page);

    // Adopt the menu bar created by PipelinePage
    auto *mb = m_page->createMenuBar(this);
    if (mb)
        setMenuBar(mb);

    statusBar()->showMessage(tr("Ready"));
}
