#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>

#include <dstools/AppInit.h>
#include <dstools/DomainInit.h>
#include <dstools/ExportFormats.h>
#include <dstools/QualityMetrics.h>
#include <dsfw/AppShell.h>
#include <dsfw/IPageActions.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/Theme.h>

#include "CleanupDialog.h"

#include "AppSettingsBackend.h"

// Page includes
#include <SlicerPage.h>
#include <MinLabelPage.h>
#include <PhonemeLabelerPage.h>
#include <PitchLabelerPage.h>
#include "pages/BuildCsvPage.h"
#include "pages/BuildDsPage.h"
#include "pages/GameAlignPage.h"

#include <SettingsPage.h>
#include <DirectoryDataSource.h>

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

    static constexpr int kDefaultWidth = 1400;
    static constexpr int kDefaultHeight = 900;

    // Register export formats and quality metrics
    static dstools::HtsLabelExportFormat htsExport;
    static dstools::QualityMetrics qualityMetrics;
    dstools::ServiceLocator::set<dstools::IExportFormat>(&htsExport);
    dstools::ServiceLocator::set<dstools::IQualityMetrics>(&qualityMetrics);

    dsfw::AppShell shell;
    shell.setWindowTitle(QStringLiteral("LabelSuite"));
    shell.resize(kDefaultWidth, kDefaultHeight);

    // ── Data source for shared pages ────────────────────────────────────

    auto *dirSource = new dstools::DirectoryDataSource(&shell);
    auto *settingsBackend = new dstools::AppSettingsBackend(&shell);

    QObject::connect(&shell, &dsfw::AppShell::workingDirectoryChanged,
                     dirSource, &dstools::DirectoryDataSource::setWorkingDirectory);

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

    // Step 9: Settings
    auto *settingsPage = new dstools::SettingsPage(settingsBackend, &shell);
    shell.addPage(settingsPage, "settings", {}, QObject::tr("Settings"));

    // ── Global menu actions (File menu — no .dsproj project management) ──

    auto *fileMenu = new QMenu(QObject::tr("&File"), &shell);

    auto *setDirAction = fileMenu->addAction(QObject::tr("Set Working Directory..."));
    QObject::connect(setDirAction, &QAction::triggered, &shell, [&]() {
        auto dir = QFileDialog::getExistingDirectory(
            &shell, QObject::tr("Select Working Directory"), shell.workingDirectory());
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
            "slicer",    "asr",      "asr_review",  "alignment", "alignment_review",
            "build_csv", "midi",     "build_ds",    "pitch_review",
        };

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

    fileMenu->addSeparator();

    auto *exitAction = fileMenu->addAction(QObject::tr("Exit"));
    exitAction->setShortcut(QKeySequence::Quit);
    QObject::connect(exitAction, &QAction::triggered, &shell, &QWidget::close);

    shell.addGlobalMenuActions({fileMenu->menuAction()});

    // ── Show ─────────────────────────────────────────────────────────────

    shell.show();

    return app.exec();
}
