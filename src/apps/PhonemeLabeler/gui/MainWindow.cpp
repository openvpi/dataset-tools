#include "MainWindow.h"
#include "PhonemeLabelerPage.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>

#include <dstools/AppSettings.h>
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

    // Connect page signals to status bar updates
    connect(m_page, &PhonemeLabelerPage::positionChanged, this, [this](double sec) {
        m_statusPosition->setText(QString::number(sec, 'f', 3) + "s");
    });
    connect(m_page, &PhonemeLabelerPage::zoomChanged, this, [this](double pps) {
        m_statusZoom->setText(QString::number(pps, 'f', 1) + " px/s");
    });
    connect(m_page, &PhonemeLabelerPage::bindingChanged, this, [this](bool enabled) {
        m_statusBinding->setText(tr("Binding: %1").arg(enabled ? "ON" : "OFF"));
    });
    connect(m_page, &PhonemeLabelerPage::fileStatusChanged, this, [this](const QString &fileName) {
        m_statusFile->setText(fileName);
    });
    connect(m_page, &PhonemeLabelerPage::fileSelected, this, [this](const QString &) {
        updateWindowTitle();
    });
    connect(m_page, &PhonemeLabelerPage::modificationChanged, this, [this](bool) {
        updateWindowTitle();
    });

    // Restore window state via native QSettings
    QSettings settings("PhonemeLabeler", "PhonemeLabeler");
    auto geom = settings.value("geometry").toByteArray();
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }
    auto state = settings.value("windowState").toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    updateWindowTitle();
}

MainWindow::~MainWindow() {
    QSettings settings("PhonemeLabeler", "PhonemeLabeler");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::openFile(const QString &path) {
    m_page->openFile(path);
}

void MainWindow::buildMenuBar() {
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));

    auto *actOpenDir = m_fileMenu->addAction(tr("Open &Directory..."), [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
            m_page->settings().get(PhonemeLabelerKeys::LastDir));
        if (!dir.isEmpty()) {
            m_page->setWorkingDirectory(dir);
        }
    });

    auto *actOpenFile = m_fileMenu->addAction(tr("Open &File..."), [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Open TextGrid File"),
            m_page->settings().get(PhonemeLabelerKeys::LastDir),
            tr("TextGrid Files (*.TextGrid *.textgrid);;All Files (*.*)"));
        if (!path.isEmpty()) {
            m_page->openFile(path);
        }
    });
    Q_UNUSED(actOpenFile)
    m_fileMenu->addSeparator();

    // Add edit actions (Save, SaveAs) from page
    auto editActs = m_page->editActions();
    // editActions() returns: [Undo, Redo, nullptr(separator), Save, SaveAs]
    enum EditActionIndex { ActUndo = 0, ActRedo = 1, /* Sep = 2, */ ActSave = 3, ActSaveAs = 4 };
    // Add Save and SaveAs to File menu
    if (editActs.size() > ActSaveAs) {
        if (editActs[ActSave]) m_fileMenu->addAction(editActs[ActSave]);
        if (editActs[ActSaveAs]) m_fileMenu->addAction(editActs[ActSaveAs]);
    }

    m_fileMenu->addSeparator();
    auto *actExit = m_fileMenu->addAction(tr("E&xit"), this, &QMainWindow::close);

    // Bind MainWindow-owned shortcuts
    auto *sm = m_page->shortcutManager();
    sm->bind(actOpenDir, PhonemeLabelerKeys::ShortcutOpen, tr("Open Directory"), tr("File"));
    sm->bind(actExit, PhonemeLabelerKeys::ShortcutExit, tr("Exit"), tr("File"));
    sm->applyAll();

    // Edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));
    if (editActs.size() > ActRedo) {
        if (editActs[ActUndo]) m_editMenu->addAction(editActs[ActUndo]);
        if (editActs[ActRedo]) m_editMenu->addAction(editActs[ActRedo]);
    }

    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));
    auto viewActs = m_page->viewActions();
    for (auto *act : viewActs) {
        if (act) {
            m_viewMenu->addAction(act);
        } else {
            m_viewMenu->addSeparator();
        }
    }

    // Add spectrogram color submenu
    m_viewMenu->addMenu(m_page->spectrogramColorMenu());

    m_viewMenu->addSeparator();
    m_viewMenu->addAction(tr("&Keyboard Shortcuts..."), [this]() {
        m_page->shortcutManager()->showEditor(this);
    });

    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(tr("&About"), [this]() {
        QMessageBox::about(this, tr("About PhonemeLabeler"),
            tr("<h3>PhonemeLabeler 0.1.0</h3>"
               "<p>A TextGrid editor for DiffSinger dataset preparation.</p>"
               "<p>Built with Qt %1 and C++17.</p>")
               .arg(QT_VERSION_STR));
    });
}

void MainWindow::buildStatusBar() {
    m_statusFile = new QLabel(tr("No file"), this);
    statusBar()->addWidget(m_statusFile);

    m_statusPosition = new QLabel("0.000s", this);
    statusBar()->addPermanentWidget(m_statusPosition);

    m_statusZoom = new QLabel("200.0 px/s", this);
    statusBar()->addPermanentWidget(m_statusZoom);

    m_statusBinding = new QLabel(tr("Binding: ON"), this);
    statusBar()->addPermanentWidget(m_statusBinding);
}

void MainWindow::updateWindowTitle() {
    QString title = tr("PhonemeLabeler");
    QString filePath = m_page->currentFilePath();
    if (!filePath.isEmpty()) {
        title += " - " + QFileInfo(filePath).fileName();
        if (m_page->hasUnsavedChanges()) {
            title += " *";
        }
    }
    setWindowTitle(title);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_page->maybeSave()) {
        QSettings settings("PhonemeLabeler", "PhonemeLabeler");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        event->accept();
    } else {
        event->ignore();
    }
}

} // namespace phonemelabeler
} // namespace dstools
