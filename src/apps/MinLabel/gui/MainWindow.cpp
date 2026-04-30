#include "MainWindow.h"
#include "MinLabelPage.h"
#include "../MinLabelKeys.h"

#include <QApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QMenuBar>
#include <QMessageBox>

#include <dstools/ShortcutManager.h>

namespace Minlabel {

    static constexpr int kDefaultWidth = 1280;
    static constexpr int kDefaultHeight = 720;

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
        setAcceptDrops(true);

        m_page = new MinLabelPage(this);
        setCentralWidget(m_page);

        buildMenuBar();

        connect(m_page, &MinLabelPage::workingDirectoryChanged, this, &MainWindow::updateWindowTitle);

        updateWindowTitle();
        resize(kDefaultWidth, kDefaultHeight);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::buildMenuBar() {
        if (auto *bar = m_page->createMenuBar(this)) {
            setMenuBar(bar);
        }
    }

    void MainWindow::updateWindowTitle() {
        setWindowTitle(m_page->windowTitle());
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
        m_page->handleDragEnter(event);
    }

    void MainWindow::dropEvent(QDropEvent *event) {
        m_page->handleDrop(event);
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        if (m_page->hasUnsavedChanges()) {
            const auto result = QMessageBox::question(
                this, tr("Unsaved Changes"),
                tr("You have unsaved changes. Do you want to save before closing?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);
            if (result == QMessageBox::Cancel) {
                event->ignore();
                return;
            }
            if (result == QMessageBox::Save) {
                m_page->save();
            }
        }

        m_page->shortcutManager()->saveAll();

        const QString dir = m_page->workingDirectory();
        if (!dir.isEmpty()) {
            m_page->settings().set(MinLabelKeys::LastDir, dir);
        }

        event->accept();
    }

} // namespace Minlabel
