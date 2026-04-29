#include "LabelerWindow.h"
#include "IPageActions.h"
#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"
#include "pages/BuildCsvPage.h"
#include "pages/BuildDsPage.h"
#include "pages/GameAlignPage.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include <dstools/Theme.h>

namespace dstools::labeler {

static const char *stepLabels[] = {
    "Slice", "ASR", "Label", "Align", "Phone", "CSV", "MIDI", "DS", "Pitch",
};

LabelerWindow::LabelerWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("DiffSinger Labeler"));

    // Central widget with horizontal layout
    auto *central = new QWidget;
    auto *hLayout = new QHBoxLayout(central);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);

    m_navigator = new StepNavigator(this);
    m_stack = new QStackedWidget(this);

    hLayout->addWidget(m_navigator);
    hLayout->addWidget(m_stack, 1);

    setCentralWidget(central);

    setupMenuBar();
    setupStatusBar();

    connect(m_navigator, &StepNavigator::stepSelected, this, &LabelerWindow::onStepChanged);

    // Select first step
    m_navigator->setCurrentRow(0);
}

void LabelerWindow::setupMenuBar() {
    auto *mb = menuBar();

    // File menu
    auto *fileMenu = mb->addMenu(tr("File(&F)"));
    fileMenu->addAction(tr("Set Working Directory..."), this, [this]() {
        auto dir = QFileDialog::getExistingDirectory(this, tr("Select Working Directory"),
                                                     m_workingDir);
        if (!dir.isEmpty())
            setWorkingDirectory(dir);
    });
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), this, &QWidget::close, QKeySequence::Quit);

    // Edit menu (dynamic)
    m_editMenu = mb->addMenu(tr("Edit(&E)"));

    // View menu (dynamic + theme)
    m_viewMenu = mb->addMenu(tr("View(&V)"));

    // Tools menu (dynamic)
    m_toolsMenu = mb->addMenu(tr("Tools(&T)"));

    // Help menu
    auto *helpMenu = mb->addMenu(tr("Help(&H)"));
    helpMenu->addAction(tr("About"), this, [this]() {
        QMessageBox::about(this, tr("About DiffSinger Labeler"),
                           tr("DiffSinger Labeler v%1\n\n"
                              "Unified dataset processing pipeline.")
                               .arg(QApplication::applicationVersion()));
    });
}

void LabelerWindow::setupStatusBar() {
    m_statusDir = new QLabel(tr("No directory"));
    m_statusStep = new QLabel;
    m_statusFiles = new QLabel;
    m_statusModified = new QLabel;

    statusBar()->addWidget(m_statusDir, 1);
    statusBar()->addPermanentWidget(m_statusStep);
    statusBar()->addPermanentWidget(m_statusFiles);
    statusBar()->addPermanentWidget(m_statusModified);
}

void LabelerWindow::setWorkingDirectory(const QString &dir) {
    m_workingDir = QDir::toNativeSeparators(dir);
    m_statusDir->setText(m_workingDir);

    // Update all created pages
    for (int i = 0; i < 9; ++i) {
        if (!m_pages[i])
            continue;
        if (auto *actions = qobject_cast<IPageActions *>(m_pages[i]))
            actions->setWorkingDirectory(dir);
    }
}

void LabelerWindow::onStepChanged(int step) {
    if (step < 0 || step >= 9)
        return;

    auto *page = ensurePage(step);
    m_stack->setCurrentWidget(page);
    m_statusStep->setText(tr("Step: %1").arg(stepLabels[step]));
    updateDynamicMenus();
}

QWidget *LabelerWindow::ensurePage(int step) {
    if (step < 0 || step >= 9)
        return nullptr;

    if (!m_pages[step]) {
        QWidget *page = nullptr;

        switch (step) {
        case 0: { // Slice — placeholder until SlicerPage integration
            auto *lbl = new QLabel(tr("Audio Slicer \u2014 Will integrate SlicerPage"));
            lbl->setAlignment(Qt::AlignCenter);
            auto f = lbl->font();
            f.setPointSize(16);
            lbl->setFont(f);
            page = lbl;
            break;
        }
        case 1: { // ASR — placeholder until LyricFAPage integration
            auto *lbl = new QLabel(tr("Lyric ASR \u2014 Will integrate LyricFAPage"));
            lbl->setAlignment(Qt::AlignCenter);
            auto f = lbl->font();
            f.setPointSize(16);
            lbl->setFont(f);
            page = lbl;
            break;
        }
        case 3: { // Align — placeholder until HubertFAPage integration
            auto *lbl = new QLabel(tr("Phoneme Alignment \u2014 Will integrate HubertFAPage"));
            lbl->setAlignment(Qt::AlignCenter);
            auto f = lbl->font();
            f.setPointSize(16);
            lbl->setFont(f);
            page = lbl;
            break;
        }
        case 5: // CSV
            page = new BuildCsvPage;
            break;
        case 6: // MIDI
            page = new GameAlignPage;
            break;
        case 7: // DS
            page = new BuildDsPage;
            break;
        default: {
            auto *placeholder = new QLabel(
                tr("Step %1 \u2014 %2\n\nNot yet implemented").arg(step + 1).arg(stepLabels[step]));
            placeholder->setAlignment(Qt::AlignCenter);
            auto f = placeholder->font();
            f.setPointSize(16);
            placeholder->setFont(f);
            page = placeholder;
            break;
        }
        }

        if (page) {
            m_stack->addWidget(page);
            m_pages[step] = page;
            // Set working directory if applicable
            if (!m_workingDir.isEmpty()) {
                if (auto *actions = qobject_cast<IPageActions *>(page))
                    actions->setWorkingDirectory(m_workingDir);
            }
        }
    }
            case 4: { // Step 5: Phone (PhonemeLabeler)
                auto *p = new dstools::phonemelabeler::PhonemeLabelerPage(this);
                if (!m_workingDir.isEmpty())
                    p->setWorkingDirectory(m_workingDir);
                page = p;
                break;
            }
            case 8: { // Step 9: Pitch (PitchLabeler)
                auto *p = new dstools::pitchlabeler::PitchLabelerPage(this);
                if (!m_workingDir.isEmpty())
                    p->setWorkingDirectory(m_workingDir);
                page = p;
                break;
            }
            default: {
                auto *placeholder = new QLabel(
                    tr("Step %1 \u2014 %2\n\nNot yet implemented").arg(step + 1).arg(stepLabels[step]));
                placeholder->setAlignment(Qt::AlignCenter);
                auto f = placeholder->font();
                f.setPointSize(16);
                placeholder->setFont(f);
                page = placeholder;
                break;
            }
        }

        m_stack->addWidget(page);
        m_pages[step] = page;
    }

    return m_pages[step];
}

void LabelerWindow::updateDynamicMenus() {
    // Clear dynamic menus
    m_editMenu->clear();
    m_viewMenu->clear();
    m_toolsMenu->clear();

    auto *currentPage = m_stack->currentWidget();
    auto *actions = qobject_cast<IPageActions *>(currentPage);

    // Edit menu: page edit actions
    if (actions) {
        for (auto *a : actions->editActions())
            m_editMenu->addAction(a);
    }
    if (m_editMenu->isEmpty())
        m_editMenu->addAction(tr("(No actions)"))->setEnabled(false);

    // View menu: page view actions + theme
    if (actions) {
        for (auto *a : actions->viewActions())
            m_viewMenu->addAction(a);
        if (!actions->viewActions().isEmpty())
            m_viewMenu->addSeparator();
    }
    dstools::Theme::instance().populateThemeMenu(m_viewMenu);

    // Tools menu: Shortcuts + page tool actions
    if (actions) {
        for (auto *a : actions->toolActions())
            m_toolsMenu->addAction(a);
    }
    if (m_toolsMenu->isEmpty())
        m_toolsMenu->addAction(tr("(No tools)"))->setEnabled(false);
}

void LabelerWindow::closeEvent(QCloseEvent *event) {
    if (hasAnyUnsavedChanges()) {
        auto ret = QMessageBox::question(
            this, tr("Unsaved Changes"),
            tr("There are unsaved changes. Are you sure you want to exit?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    event->accept();
}

bool LabelerWindow::hasAnyUnsavedChanges() const {
    for (int i = 0; i < 9; ++i) {
        if (!m_pages[i])
            continue;
        if (auto *actions = qobject_cast<IPageActions *>(m_pages[i]))
            if (actions->hasUnsavedChanges())
                return true;
    }
    return false;
}

} // namespace dstools::labeler
