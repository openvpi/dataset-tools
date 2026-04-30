#include "MainWindow.h"
#include "PhonemeLabelerPage.h"

#include <QCloseEvent>
#include <QStatusBar>
#include <QToolBar>

#include <dsfw/AppSettings.h>
#include "../PhonemeLabelerKeys.h"

namespace dstools {
namespace phonemelabeler {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_page(new PhonemeLabelerPage(this))
{
    setCentralWidget(m_page);

    // Add the page's toolbar to the main window
    addToolBar(m_page->toolbar());

    buildMenuBar();
    buildStatusBar();

    // Connect page signals to window title updates
    connect(m_page, &PhonemeLabelerPage::fileSelected, this, [this](const QString &) {
        updateWindowTitle();
    });
    connect(m_page, &PhonemeLabelerPage::modificationChanged, this, [this](bool) {
        updateWindowTitle();
    });

    // Restore window state
    auto geomB64 = m_page->settings().get(PhonemeLabelerKeys::WindowGeometry);
    if (!geomB64.isEmpty())
        restoreGeometry(QByteArray::fromBase64(geomB64.toUtf8()));
    auto stateB64 = m_page->settings().get(PhonemeLabelerKeys::WindowState);
    if (!stateB64.isEmpty())
        restoreState(QByteArray::fromBase64(stateB64.toUtf8()));

    updateWindowTitle();
}

MainWindow::~MainWindow() {
    m_page->settings().set(PhonemeLabelerKeys::WindowGeometry,
                           QString::fromLatin1(saveGeometry().toBase64()));
    m_page->settings().set(PhonemeLabelerKeys::WindowState,
                           QString::fromLatin1(saveState().toBase64()));
}

void MainWindow::openFile(const QString &path) {
    m_page->openFile(path);
}

void MainWindow::buildMenuBar() {
    auto *mb = m_page->createMenuBar(this);
    if (mb) {
        setMenuBar(mb);
    }
}

void MainWindow::buildStatusBar() {
    auto *content = m_page->createStatusBarContent(this);
    if (content) {
        statusBar()->addWidget(content, 1);
    }
}

void MainWindow::updateWindowTitle() {
    setWindowTitle(m_page->windowTitle());
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_page->maybeSave()) {
        m_page->settings().set(PhonemeLabelerKeys::WindowGeometry,
                               QString::fromLatin1(saveGeometry().toBase64()));
        m_page->settings().set(PhonemeLabelerKeys::WindowState,
                               QString::fromLatin1(saveState().toBase64()));
        event->accept();
    } else {
        event->ignore();
    }
}

} // namespace phonemelabeler
} // namespace dstools
