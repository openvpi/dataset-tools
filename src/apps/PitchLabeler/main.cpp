#include "gui/PitchLabelerPage.h"

#include <QApplication>

#include <dstools/AppInit.h>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("DiffSinger Pitch Labeler");
    app.setOrganizationName("DiffSinger");
    app.setApplicationVersion("0.1.0");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    auto *page = new dstools::pitchlabeler::PitchLabelerPage(&shell);
    shell.addPage(page, "pitch-labeler", {}, "PitchLabeler");
    shell.setWindowTitle(QStringLiteral("DiffSinger 音高标注器"));
    shell.setMinimumSize(1024, 680);
    shell.resize(1440, 900);
    shell.show();

    // Sync page-driven title changes back to shell
    auto updateTitle = [&]() { shell.setWindowTitle(page->windowTitle()); };
    QObject::connect(page, &dstools::pitchlabeler::PitchLabelerPage::modificationChanged,
                     &shell, [&](bool) { updateTitle(); });
    QObject::connect(page, &dstools::pitchlabeler::PitchLabelerPage::fileLoaded,
                     &shell, [&](const QString &) { updateTitle(); });
    QObject::connect(page, &dstools::pitchlabeler::PitchLabelerPage::fileSaved,
                     &shell, [&](const QString &) { updateTitle(); });
    QObject::connect(page, &dstools::pitchlabeler::PitchLabelerPage::workingDirectoryChanged,
                     &shell, [&](const QString &) { updateTitle(); });

    // Save config on shutdown (before unsaved-changes prompt in AppShell::closeEvent)
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        page->saveConfig();
    });

    return app.exec();
}
