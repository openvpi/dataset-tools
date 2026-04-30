#include "MainWindow.h"
#include "PitchLabelerPage.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

namespace dstools {
    namespace pitchlabeler {

        MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
            setWindowTitle(QStringLiteral("DiffSinger 音高标注器"));
            setMinimumSize(1024, 680);
            resize(1440, 900);

            m_page = new PitchLabelerPage(this);
            setCentralWidget(m_page);

            // Delegate menu bar creation to page
            setMenuBar(m_page->createMenuBar(this));

            // Delegate status bar content to page
            if (auto *content = m_page->createStatusBarContent(statusBar())) {
                statusBar()->addWidget(content, 1);
            }

            // Connect page signals to window title updates
            connect(m_page, &PitchLabelerPage::modificationChanged, this, [this](bool) {
                updateWindowTitle();
            });
            connect(m_page, &PitchLabelerPage::fileLoaded, this, [this](const QString &path) {
                updateWindowTitle();
                emit fileLoaded(path);
            });
            connect(m_page, &PitchLabelerPage::fileSaved, this, [this](const QString &path) {
                updateWindowTitle();
                statusBar()->showMessage(QStringLiteral("已保存: ") + QFileInfo(path).fileName(), 3000);
                emit fileSaved(path);
            });
            connect(m_page, &PitchLabelerPage::workingDirectoryChanged, this, [this](const QString &) {
                updateWindowTitle();
            });

            updateWindowTitle();
        }

        MainWindow::~MainWindow() = default;

        void MainWindow::updateWindowTitle() {
            setWindowTitle(m_page->windowTitle());
        }

        void MainWindow::closeEvent(QCloseEvent *event) {
            // Save config
            m_page->saveConfig();

            // Check for unsaved changes
            if (m_page->hasUnsavedChanges()) {
                auto ret = QMessageBox::question(this, QStringLiteral("未保存的更改"),
                                                 QStringLiteral("关闭前是否保存？"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                if (ret == QMessageBox::Save) {
                    if (!m_page->save()) {
                        event->ignore();
                        return;
                    }
                } else if (ret == QMessageBox::Cancel) {
                    event->ignore();
                    return;
                }
            }
            event->accept();
        }

    } // namespace pitchlabeler
} // namespace dstools
