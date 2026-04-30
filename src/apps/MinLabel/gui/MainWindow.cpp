#include "MainWindow.h"
#include "MinLabelPage.h"
#include "../MinLabelKeys.h"

#include <QApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>

#include <dstools/ShortcutManager.h>
#include <dstools/Theme.h>

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
        m_fileMenu = new QMenu("File(&F)", this);
        m_fileMenu->addAction(m_page->browseAction());
        m_fileMenu->addAction(m_page->exportAction());
        m_fileMenu->addAction(m_page->convertAction());

        m_editMenu = new QMenu("Edit(&E)", this);
        m_editMenu->addAction(m_page->nextAction());
        m_editMenu->addAction(m_page->prevAction());

        m_editMenu->addSeparator();
        {
            auto *shortcutAction = new QAction("Shortcut Settings...", this);
            m_editMenu->addAction(shortcutAction);
            connect(shortcutAction, &QAction::triggered, this, [this]() {
                m_page->shortcutManager()->showEditor(this);
            });
        }

        m_playMenu = new QMenu("Playback(&P)", this);
        m_playMenu->addAction(m_page->playAction());

        auto *aboutAppAction = new QAction(QString("About %1").arg(QApplication::applicationName()), this);
        auto *aboutQtAction = new QAction("About Qt", this);

        m_helpMenu = new QMenu("Help(&H)", this);
        m_helpMenu->addAction(aboutAppAction);
        m_helpMenu->addAction(aboutQtAction);

        connect(m_helpMenu, &QMenu::triggered, this, [this](const QAction *action) {
            if (action->text().startsWith("About Qt")) {
                QMessageBox::aboutQt(this);
            } else {
                QMessageBox::information(
                    this, QApplication::applicationName(),
                    QString("%1 %2, Copyright OpenVPI.").arg(QApplication::applicationName(), APP_VERSION));
            }
        });

        auto *bar = menuBar();
        bar->addMenu(m_fileMenu);
        bar->addMenu(m_editMenu);
        bar->addMenu(m_playMenu);

        auto *viewMenu = new QMenu("View(&V)", this);
        dstools::Theme::instance().populateThemeMenu(viewMenu);
        bar->addMenu(viewMenu);

        bar->addMenu(m_helpMenu);
    }

    void MainWindow::updateWindowTitle() {
        const QString dir = m_page->workingDirectory();
        setWindowTitle(dir.isEmpty()
                           ? QApplication::applicationName()
                           : QString("%1 - %2").arg(QApplication::applicationName(),
                                                    QDir::toNativeSeparators(QFileInfo(dir).baseName())));
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
        const QMimeData *mime = event->mimeData();
        if (mime->hasUrls()) {
            auto urls = mime->urls();
            QStringList filenames;
            for (auto &url : urls) {
                if (url.isLocalFile()) {
                    filenames.append(url.toLocalFile());
                }
            }
            bool ok = false;
            if (filenames.size() == 1) {
                if (const QString &filename = filenames.front(); QFile::exists(filename)) {
                    ok = true;
                }
            }
            if (ok) {
                event->acceptProposedAction();
            }
        }
    }

    void MainWindow::dropEvent(QDropEvent *event) {
        if (const QMimeData *mime = event->mimeData(); mime->hasUrls()) {
            auto urls = mime->urls();
            QStringList filenames;
            for (auto &url : urls) {
                if (url.isLocalFile()) {
                    filenames.append(url.toLocalFile());
                }
            }
            bool ok = false;
            if (filenames.size() == 1) {
                if (const QString &filename = filenames.front(); QFile::exists(filename)) {
                    ok = true;
                    const QFileInfo fi(filename);
                    m_page->setWorkingDirectory(fi.isDir() ? filename : fi.absolutePath());
                }
            }
            if (ok) {
                event->acceptProposedAction();
            }
        }
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
