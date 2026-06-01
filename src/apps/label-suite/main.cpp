#include "CleanupDialog.h"

#include <AppSettingsBackend.h>
#include <ModelProviderInit.h>

#include <dsfw/FileDialogHelper.h>

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QMenu>
#include <QMessageBox>
#include <dsfw/AppPaths.h>
#include <dsfw/AppShell.h>
#include <dsfw/PathUtils.h>
#include <dstools/ModelManager.h>
#include <dsfw/IPageActions.h>
#include <dsfw/Theme.h>
#include <dstools/AppInit.h>
#include <dstools/DomainInit.h>
#include <SettingsPage.h>

// Page includes
#include "PitchLabelerPage.h"
#include "pages/BuildCsvPage.h"
#include "pages/BuildDsPage.h"
#include "pages/GameAlignPage.h"

#include <DirectoryDataSource.h>
#include <MinLabelPage.h>
#include <PageDescriptors.h>
#include <PageFactory.h>
#include <PhonemeLabelerPage.h>
#include <SlicerPage.h>

using namespace dstools::labeler;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("LabelSuite");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    dstools::registerDomainFormatAdapters();
    dsfw::Theme::instance().init(app);

    auto *modelManager = &dstools::ModelManager::instance();
    if (modelManager)
        dstools::registerModelProviders(*modelManager);

    static constexpr int kDefaultWidth = 1400;
    static constexpr int kDefaultHeight = 900;

    // ── AppShell setup ────────────────────────────────────────────────────

    dsfw::AppShell shell;
    shell.setWindowTitle(QStringLiteral("LabelSuite"));
    shell.resize(kDefaultWidth, kDefaultHeight);

    if (dstools::AppInit::wasPreviousCrash()) {
        QMessageBox::information(
            &shell, QObject::tr("Abnormal Exit"),
            QObject::tr("The previous session ended unexpectedly (crash or forced termination).\n\n"
                        "Your annotation data is protected by auto-save.\n"
                        "Log files are saved in %1/logs for troubleshooting.")
                .arg(dsfw::AppPaths::dataDir()),
            QMessageBox::Ok);
    }

    // ── Data source for shared pages ────────────────────────────────────

    auto *dirSource = new dstools::DirectoryDataSource(&shell);
    auto *settingsBackend = new dstools::AppSettingsBackend(&shell);

    QObject::connect(&shell, &dsfw::AppShell::workingDirectoryChanged, dirSource,
                     &dstools::DirectoryDataSource::setWorkingDirectory);

    // ── Create and register all 10 pages ──────────────────────────────────

    // Step 0: Slice — shared SlicerPage
    auto *slicePage = new dstools::SlicerPage(&shell);
    shell.addPage(slicePage, "slice", {}, QObject::tr("Slice"));

    // Step 1: ASR — shared MinLabelPage with ASR functionality
    auto *asrPage = new dstools::MinLabelPage(&shell);
    asrPage->setDataSource(dirSource, settingsBackend);
    shell.addPage(asrPage, "asr", {}, QObject::tr("ASR"));

    // Step 2: Label (MinLabel) — shared page
    auto *labelPage = new dstools::MinLabelPage(&shell);
    labelPage->setDataSource(dirSource, settingsBackend);
    shell.addPage(labelPage, "label", {}, QObject::tr("Label"));

    // Step 3: Align — shared PhonemeLabelerPage with FA functionality
    auto *alignPage = new dstools::PhonemeLabelerPage(&shell);
    alignPage->setDataSource(dirSource, settingsBackend);
    shell.addPage(alignPage, "align", {}, QObject::tr("Align"));

    // Step 4: Phone (PhonemeLabeler) — shared page
    auto *phonePage = new dstools::PhonemeLabelerPage(&shell);
    phonePage->setDataSource(dirSource, settingsBackend);
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

    // Step 8: Pitch (PitchLabeler) — shared page
    auto *pitchPage = new dstools::PitchLabelerPage(&shell);
    pitchPage->setDataSource(dirSource, settingsBackend);
    shell.addPage(pitchPage, "pitch", {}, QObject::tr("Pitch"));

    // Steps 9-10: Settings + Log (utility pages)
    static const dstools::SettingsPageDescriptor settingsPageDesc;
    static const dstools::LogPageDescriptor logPageDesc;
    dstools::PageFactory::registerPages(&shell, nullptr, settingsBackend, {&settingsPageDesc, &logPageDesc});

    auto *settingsPage = shell.findChild<dstools::SettingsPage *>(QString(), Qt::FindDirectChildrenOnly);

    if (modelManager) {
        QObject::connect(settingsPage, &dstools::SettingsPage::modelReloadRequested, modelManager,
                         &dstools::ModelManager::invalidateModel);
    }

    // ── Global menu actions (File menu — no .dsproj project management) ──

    auto *fileMenu = new QMenu(QObject::tr("&File"), &shell);

    auto *setDirAction = fileMenu->addAction(QObject::tr("Set Working Directory..."));
    QObject::connect(setDirAction, &QAction::triggered, &shell, [&]() {
        auto dir = dsfw::FileDialogHelper::getExistingDirectory(
            {&shell, QObject::tr("Select Working Directory"), shell.workingDirectory()});
        if (!dir.isEmpty())
            shell.setWorkingDirectory(dir);
    });

    fileMenu->addSeparator();

    auto *cleanupAction = fileMenu->addAction(QObject::tr("Clean Working Directory..."));
    QObject::connect(cleanupAction, &QAction::triggered, &shell, [&]() {
        auto workingDir = shell.workingDirectory();
        if (workingDir.isEmpty()) {
            QMessageBox::warning(&shell, QObject::tr("No Working Directory"),
                                 QObject::tr("Please set a working directory first."));
            return;
        }

        CleanupDialog dlg(workingDir, &shell);
        if (dlg.exec() != QDialog::Accepted)
            return;

        auto steps = dlg.selectedSteps();
        if (steps.isEmpty())
            return;

        static const char *stepDirs[] = {
            "slicer",    "asr",  "asr_review", "alignment",    "alignment_review",
            "build_csv", "midi", "build_ds",   "pitch_review",
        };

        int totalFiles = 0;
        qint64 totalSize = 0;
        QDir base(dsfw::PathUtils::fromStdPath(
        dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(workingDir), "dstemp")));

        for (int idx : steps) {
            QDir stepDir(base.filePath(stepDirs[idx]));
            if (!stepDir.exists())
                continue;
            QDirIterator it(stepDir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                totalSize += it.fileInfo().size();
                if (QFile::remove(it.filePath()))
                    ++totalFiles;
            }
        }

        QMessageBox::information(
            &shell, QObject::tr("Cleanup Complete"),
            QObject::tr("Deleted %1 files (%2 MB)").arg(totalFiles).arg(totalSize / (1024 * 1024)));
    });

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction(QObject::tr("Exit"));
    exitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(exitAction, &QAction::triggered, &shell, &QWidget::close);

    shell.addGlobalMenuActions({fileMenu->menuAction()});

    // ── Show ─────────────────────────────────────────────────────────────

    shell.show();

    return app.exec();
}
