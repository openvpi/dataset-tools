#include "MainWindow.h"
#include "PitchLabelerPage.h"
#include "../PitchLabelerKeys.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include <dsfw/Theme.h>
#include <dstools/ShortcutManager.h>

namespace dstools {
    namespace pitchlabeler {

        MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
            setWindowTitle(QStringLiteral("DiffSinger 音高标注器"));
            setMinimumSize(1024, 680);
            resize(1440, 900);

            m_page = new PitchLabelerPage(this);
            setCentralWidget(m_page);

            buildMenuBar();
            buildStatusBar();

            // Bind shortcuts via page's ShortcutManager
            auto *sm = m_page->shortcutManager();
            sm->bind(m_actOpenDir, PitchLabelerKeys::ShortcutOpen, QStringLiteral("Open Directory"), QStringLiteral("File"));
            sm->bind(m_page->saveAction(), PitchLabelerKeys::ShortcutSave, QStringLiteral("Save"), QStringLiteral("File"));
            sm->bind(m_page->saveAllAction(), PitchLabelerKeys::ShortcutSaveAll, QStringLiteral("Save All"), QStringLiteral("File"));
            sm->bind(m_actExit, PitchLabelerKeys::ShortcutExit, QStringLiteral("Exit"), QStringLiteral("File"));
            sm->bind(m_page->undoAction(), PitchLabelerKeys::ShortcutUndo, QStringLiteral("Undo"), QStringLiteral("Edit"));
            sm->bind(m_page->redoAction(), PitchLabelerKeys::ShortcutRedo, QStringLiteral("Redo"), QStringLiteral("Edit"));
            sm->bind(m_page->zoomInAction(), PitchLabelerKeys::ShortcutZoomIn, QStringLiteral("Zoom In"), QStringLiteral("View"));
            sm->bind(m_page->zoomOutAction(), PitchLabelerKeys::ShortcutZoomOut, QStringLiteral("Zoom Out"), QStringLiteral("View"));
            sm->bind(m_page->zoomResetAction(), PitchLabelerKeys::ShortcutZoomReset, QStringLiteral("Reset Zoom"), QStringLiteral("View"));
            sm->bind(m_page->abCompareAction(), PitchLabelerKeys::ShortcutABCompare, QStringLiteral("A/B Compare"), QStringLiteral("Tools"));
            sm->applyAll();

            // Connect page signals to status bar / title updates
            connect(m_page, &PitchLabelerPage::fileStatusChanged, this, [this](const QString &name) {
                if (name.isEmpty()) {
                    m_statusFile->setText(QStringLiteral("未加载文件"));
                    m_fileLoaded = false;
                } else {
                    m_statusFile->setText(name);
                    m_fileLoaded = true;
                }
            });
            connect(m_page, &PitchLabelerPage::positionChanged, this, [this](double sec) {
                int minutes = static_cast<int>(sec) / 60;
                double seconds = sec - minutes * 60;
                m_statusPosition->setText(
                    QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
            });
            connect(m_page, &PitchLabelerPage::zoomChanged, this, [this](int percent) {
                m_statusZoom->setText(QString::number(percent) + "%");
            });
            connect(m_page, &PitchLabelerPage::noteCountChanged, this, [this](int count) {
                m_statusNotes->setText(QStringLiteral("音符数: ") + QString::number(count));
            });
            connect(m_page, &PitchLabelerPage::toolModeChanged, this, [this](int mode) {
                switch (mode) {
                    case 0: m_statusTool->setText(QStringLiteral("工具: 选择")); break;
                    case 1: m_statusTool->setText(QStringLiteral("工具: 颤音调制")); break;
                    case 2: m_statusTool->setText(QStringLiteral("工具: 音高偏移")); break;
                    default: break;
                }
            });
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

        void MainWindow::buildMenuBar() {
            auto *bar = menuBar();

            // ---- 文件 ----
            m_fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));

            m_actOpenDir = new QAction(QStringLiteral("打开工作目录(&O)..."), this);
            m_actOpenDir->setStatusTip(QStringLiteral("打开包含 .ds 文件的目录"));
            m_fileMenu->addAction(m_actOpenDir);
            connect(m_actOpenDir, &QAction::triggered, this, &MainWindow::openDirectory);

            m_actCloseDir = new QAction(QStringLiteral("关闭工作目录(&C)"), this);
            m_actCloseDir->setStatusTip(QStringLiteral("关闭当前工作目录"));
            m_fileMenu->addAction(m_actCloseDir);
            connect(m_actCloseDir, &QAction::triggered, m_page, &PitchLabelerPage::closeDirectory);

            m_fileMenu->addSeparator();

            // Save/SaveAll actions are owned by the page; add them to menu
            m_fileMenu->addAction(m_page->saveAction());
            m_fileMenu->addAction(m_page->saveAllAction());

            m_fileMenu->addSeparator();

            m_actExit = new QAction(QStringLiteral("退出(&X)"), this);
            m_actExit->setStatusTip(QStringLiteral("退出应用程序"));
            m_fileMenu->addAction(m_actExit);
            connect(m_actExit, &QAction::triggered, this, &QMainWindow::close);

            // ---- 编辑 ----
            m_editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
            m_editMenu->addAction(m_page->undoAction());
            m_editMenu->addAction(m_page->redoAction());

            // ---- 视图 ----
            m_viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
            m_viewMenu->addAction(m_page->zoomInAction());
            m_viewMenu->addAction(m_page->zoomOutAction());
            m_viewMenu->addAction(m_page->zoomResetAction());
            m_viewMenu->addSeparator();
            dsfw::Theme::instance().populateThemeMenu(m_viewMenu);

            // ---- 工具 ----
            m_toolsMenu = bar->addMenu(QStringLiteral("工具(&T)"));
            m_toolsMenu->addAction(m_page->abCompareAction());
            m_toolsMenu->addSeparator();
            {
                auto *shortcutAction = new QAction(QStringLiteral("快捷键设置(&K)..."), this);
                m_toolsMenu->addAction(shortcutAction);
                connect(shortcutAction, &QAction::triggered, this, [this]() {
                    m_page->shortcutManager()->showEditor(this);
                });
            }

            // ---- 帮助 ----
            m_helpMenu = bar->addMenu(QStringLiteral("帮助(&H)"));
            m_actAbout = new QAction(QStringLiteral("关于(&A)"), this);
            m_actAbout->setStatusTip(QStringLiteral("关于 DiffSinger 音高标注器"));
            m_helpMenu->addAction(m_actAbout);
            connect(m_actAbout, &QAction::triggered, this, []() {
                QMessageBox::about(nullptr, QStringLiteral("关于 DiffSinger 音高标注器"),
                                   QStringLiteral("DiffSinger 音高标注器\n版本 0.1.0\n\n"
                                   "DiffSinger 数据标注工作流的音高标注编辑器。"));
            });
        }

        void MainWindow::buildStatusBar() {
            auto *sb = statusBar();

            m_statusFile = new QLabel(QStringLiteral("未加载文件"));
            m_statusFile->setMinimumWidth(160);
            sb->addWidget(m_statusFile);

            m_statusPosition = new QLabel("00:00.000");
            m_statusPosition->setMinimumWidth(100);
            sb->addWidget(m_statusPosition);

            m_statusZoom = new QLabel("100%");
            m_statusZoom->setMinimumWidth(60);
            sb->addWidget(m_statusZoom);

            m_statusTool = new QLabel(QStringLiteral("工具: 选择"));
            m_statusTool->setMinimumWidth(100);
            sb->addWidget(m_statusTool);

            m_statusNotes = new QLabel(QStringLiteral("音符数: 0"));
            m_statusNotes->setMinimumWidth(80);
            sb->addPermanentWidget(m_statusNotes);
        }

        void MainWindow::updateWindowTitle() {
            QString title = QStringLiteral("DiffSinger 音高标注器");
            if (!m_page->workingDirectory().isEmpty()) {
                if (m_fileLoaded) {
                    QString statusText = m_statusFile->text();
                    title = statusText + (m_page->hasUnsavedChanges() ? " *" : "") + " - " + title;
                }
            }
            setWindowTitle(title);
        }

        void MainWindow::openDirectory() {
            QString dir =
                QFileDialog::getExistingDirectory(this, QStringLiteral("打开工作目录"), "", QFileDialog::ShowDirsOnly);

            if (dir.isEmpty())
                return;

            m_page->setWorkingDirectory(dir);
            updateWindowTitle();
            statusBar()->showMessage(QStringLiteral("已打开: ") + dir, 3000);
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
