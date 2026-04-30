#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include <dstools/AppInit.h>
#include <dstools/DsProject.h>
#include <dsfw/AppShell.h>
#include <dsfw/IPageActions.h>
#include <dsfw/Theme.h>

#include "CleanupDialog.h"
#include "TaskWindowAdapter.h"

// Page includes
#include "SlicerPage.h"
#include "LyricFAPage.h"
#include "HubertFAPage.h"
#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"
#include "pages/BuildCsvPage.h"
#include "pages/BuildDsPage.h"
#include "pages/GameAlignPage.h"

using namespace dstools::labeler;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DiffSinger Labeler");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    static constexpr int kDefaultWidth = 1400;
    static constexpr int kDefaultHeight = 900;

    dsfw::AppShell shell;
    shell.setWindowTitle(QObject::tr("DiffSinger Labeler"));
    shell.resize(kDefaultWidth, kDefaultHeight);

    // ── Create and register all 9 pages ──────────────────────────────────

    // Step 0: Slice
    auto *slicerAdapter = new TaskWindowAdapter(new SlicerPage(&shell), &shell);
    shell.addPage(slicerAdapter, "slice", {}, QObject::tr("Slice"));

    // Step 1: ASR
    auto *asrAdapter = new TaskWindowAdapter(new LyricFAPage(&shell), &shell);
    shell.addPage(asrAdapter, "asr", {}, QObject::tr("ASR"));

    // Step 2: Label (MinLabel)
    auto *labelPage = new Minlabel::MinLabelPage(&shell);
    shell.addPage(labelPage, "label", {}, QObject::tr("Label"));

    // Step 3: Align (HubertFA)
    auto *alignAdapter = new TaskWindowAdapter(new HubertFAPage(&shell), &shell);
    shell.addPage(alignAdapter, "align", {}, QObject::tr("Align"));

    // Step 4: Phone (PhonemeLabeler)
    auto *phonePage = new dstools::phonemelabeler::PhonemeLabelerPage(&shell);
    shell.addPage(phonePage, "phone", {}, QObject::tr("Phone"));

    // Step 5: CSV
    auto *csvPage = new BuildCsvPage(&shell);
    shell.addPage(csvPage, "csv", {}, QObject::tr("CSV"));

    // Step 6: MIDI
    auto *midiPage = new GameAlignPage(&shell);
    shell.addPage(midiPage, "midi", {}, QObject::tr("MIDI"));

    // Step 7: DS
    auto *dsPage = new BuildDsPage(&shell);
    shell.addPage(dsPage, "ds", {}, QObject::tr("DS"));

    // Step 8: Pitch (PitchLabeler)
    auto *pitchPage = new dstools::pitchlabeler::PitchLabelerPage(&shell);
    shell.addPage(pitchPage, "pitch", {}, QObject::tr("Pitch"));

    // ── Project management (local state) ─────────────────────────────────

    dstools::DsProject *project = nullptr;

    // Clean up project on shell destruction
    QObject::connect(&shell, &QObject::destroyed, [&project]() {
        delete project;
        project = nullptr;
    });

    // Helper: load project from path
    auto loadProject = [&](const QString &path) {
        QString error;
        auto *proj = new dstools::DsProject(dstools::DsProject::load(path, error));
        if (!error.isEmpty()) {
            QMessageBox::warning(&shell, QObject::tr("Open Project"),
                                 QObject::tr("Failed to load project:\n%1").arg(error));
            delete proj;
            return;
        }
        delete project;
        project = proj;

        if (!project->workingDirectory().isEmpty())
            shell.setWorkingDirectory(project->workingDirectory());

        shell.setWindowTitle(
            QObject::tr("DiffSinger Labeler — %1").arg(QFileInfo(path).fileName()));
    };

    // Helper: save project
    auto saveProjectAs = [&]() {
        auto path = QFileDialog::getSaveFileName(
            &shell, QObject::tr("Save Project As"), QString(),
            QObject::tr("DiffSinger Project (*.dsproj)"));
        if (path.isEmpty())
            return;

        if (!project)
            project = new dstools::DsProject;

        project->setWorkingDirectory(shell.workingDirectory());

        QString error;
        if (!project->save(path, error)) {
            QMessageBox::warning(&shell, QObject::tr("Save Project"),
                                 QObject::tr("Failed to save project:\n%1").arg(error));
        } else {
            shell.setWindowTitle(
                QObject::tr("DiffSinger Labeler — %1").arg(QFileInfo(path).fileName()));
        }
    };

    auto saveProject = [&]() {
        if (!project) {
            saveProjectAs();
            return;
        }

        project->setWorkingDirectory(shell.workingDirectory());

        QString error;
        if (!project->save(error)) {
            QMessageBox::warning(&shell, QObject::tr("Save Project"),
                                 QObject::tr("Failed to save project:\n%1").arg(error));
        }
    };

    // ── Global menu actions (File menu) ──────────────────────────────────

    QList<QAction *> globalActions;

    auto *openAction = new QAction(QObject::tr("Open Project..."), &shell);
    QObject::connect(openAction, &QAction::triggered, &shell, [&]() {
        auto path = QFileDialog::getOpenFileName(
            &shell, QObject::tr("Open Project"), QString(),
            QObject::tr("DiffSinger Project (*.dsproj)"));
        if (!path.isEmpty())
            loadProject(path);
    });
    globalActions << openAction;

    auto *saveAction = new QAction(QObject::tr("Save Project"), &shell);
    saveAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    QObject::connect(saveAction, &QAction::triggered, &shell, [&]() { saveProject(); });
    globalActions << saveAction;

    auto *saveAsAction = new QAction(QObject::tr("Save Project As..."), &shell);
    QObject::connect(saveAsAction, &QAction::triggered, &shell, [&]() { saveProjectAs(); });
    globalActions << saveAsAction;

    auto *separator1 = new QAction(&shell);
    separator1->setSeparator(true);
    globalActions << separator1;

    auto *setDirAction = new QAction(QObject::tr("Set Working Directory..."), &shell);
    QObject::connect(setDirAction, &QAction::triggered, &shell, [&]() {
        auto dir = QFileDialog::getExistingDirectory(
            &shell, QObject::tr("Select Working Directory"), shell.workingDirectory());
        if (!dir.isEmpty())
            shell.setWorkingDirectory(dir);
    });
    globalActions << setDirAction;

    auto *separator2 = new QAction(&shell);
    separator2->setSeparator(true);
    globalActions << separator2;

    auto *cleanupAction = new QAction(QObject::tr("Clean Working Directory..."), &shell);
    QObject::connect(cleanupAction, &QAction::triggered, &shell, [&]() {
        auto workingDir = shell.workingDirectory();
        if (workingDir.isEmpty()) {
            QMessageBox::warning(&shell, QObject::tr("No Working Directory"),
                                 QObject::tr("Please set a working directory first."));
            return;
        }

        static const char *stepDirs[] = {
            "slicer",    "asr",      "asr_review",  "alignment", "alignment_review",
            "build_csv", "midi",     "build_ds",    "pitch_review",
        };

        CleanupDialog dlg(workingDir, &shell);
        if (dlg.exec() != QDialog::Accepted)
            return;

        auto steps = dlg.selectedSteps();
        if (steps.isEmpty())
            return;

        int totalFiles = 0;
        qint64 totalSize = 0;
        QDir base(workingDir + QStringLiteral("/dstemp"));

        for (int idx : steps) {
            QDir stepDir(base.filePath(stepDirs[idx]));
            if (!stepDir.exists())
                continue;
            QDirIterator it(stepDir.absolutePath(), QDir::Files,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                totalSize += it.fileInfo().size();
                if (QFile::remove(it.filePath()))
                    ++totalFiles;
            }
        }

        QMessageBox::information(
            &shell, QObject::tr("Cleanup Complete"),
            QObject::tr("Deleted %1 files (%2 MB)")
                .arg(totalFiles)
                .arg(totalSize / (1024 * 1024)));
    });
    globalActions << cleanupAction;

    auto *separator3 = new QAction(&shell);
    separator3->setSeparator(true);
    globalActions << separator3;

    auto *exitAction = new QAction(QObject::tr("Exit"), &shell);
    exitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(exitAction, &QAction::triggered, &shell, &QWidget::close);
    globalActions << exitAction;

    shell.addGlobalMenuActions(globalActions);

    // ── Show and handle command-line argument ────────────────────────────

    shell.show();

    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive))
            loadProject(arg);
    }

    return app.exec();
}
