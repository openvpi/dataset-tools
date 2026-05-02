#include <QApplication>
#include <QFileInfo>
#include <QLabel>

#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>
#include <dstools/AppInit.h>
#include <dstools/DomainInit.h>

#include "WelcomePage.h"
#include "SettingsPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DsLabeler");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    dstools::registerDomainFormatAdapters();
    dsfw::Theme::instance().init(app);

    static constexpr int kDefaultWidth = 1400;
    static constexpr int kDefaultHeight = 900;

    dsfw::AppShell shell;
    shell.setWindowTitle(QStringLiteral("DsLabeler — DiffSinger Dataset Labeler"));
    shell.resize(kDefaultWidth, kDefaultHeight);

    // Project state (owned by main, lifetime = app)
    dstools::DsProject *project = nullptr;

    // Page 1: 工程 (Welcome)
    auto *welcomePage = new dstools::WelcomePage(&shell);
    shell.addPage(welcomePage, "welcome", {}, QStringLiteral("工程"));

    // Page 2: 设置 (Settings)
    auto *settingsPage = new dstools::SettingsPage(&shell);
    shell.addPage(settingsPage, "settings", {}, QStringLiteral("设置"));

    // Future pages (M.3.5-M.3.8) will be added here:
    // shell.addPage(dsMinLabelPage, "minlabel", {}, "歌词");
    // shell.addPage(dsPhonemeLabelerPage, "phoneme", {}, "音素");
    // shell.addPage(dsPitchLabelerPage, "pitch", {}, "音高");
    // shell.addPage(exportPage, "export", {}, "导出");

    // Handle project load from WelcomePage
    QObject::connect(welcomePage, &dstools::WelcomePage::projectLoaded,
                     &shell, [&](dstools::DsProject *proj, const QString &path) {
        delete project;
        project = proj;

        settingsPage->setProject(project);

        shell.setWindowTitle(
            QStringLiteral("DsLabeler — %1").arg(QFileInfo(path).fileName()));

        // Switch to settings page after loading
        shell.setCurrentPage(1);
    });

    // Clean up
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (project) {
            settingsPage->applyToProject();
            QString error;
            project->save(error);
        }
        delete project;
        project = nullptr;
    });

    shell.show();

    // Handle command-line .dsproj argument
    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive)) {
            welcomePage->projectLoaded(nullptr, {}); // trigger load via welcome page
            // Actually use the WelcomePage's loadProject indirectly
            QString error;
            auto *proj = new dstools::DsProject(dstools::DsProject::load(arg, error));
            if (error.isEmpty()) {
                delete project;
                project = proj;
                settingsPage->setProject(project);
                shell.setWindowTitle(
                    QStringLiteral("DsLabeler — %1").arg(QFileInfo(arg).fileName()));
                shell.setCurrentPage(1);
            } else {
                delete proj;
            }
        }
    }

    return app.exec();
}
