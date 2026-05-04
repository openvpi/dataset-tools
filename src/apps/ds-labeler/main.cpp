#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>

#include <dsfw/AppShell.h>
#include <dsfw/IModelManager.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/Theme.h>
#include <dstools/AppInit.h>
#include <dstools/DomainInit.h>
#include <dstools/PinyinG2PProvider.h>

#include <cpp-pinyin/G2pglobal.h>

#include "DsSlicerPage.h"
#include "ExportPage.h"
#include "ProjectDataSource.h"
#include <AppSettingsBackend.h>
#include <ModelProviderInit.h>
#include "WelcomePage.h"

#include <MinLabelPage.h>
#include <PhonemeLabelerPage.h>
#include <PitchLabelerPage.h>
#include <SettingsPage.h>

#include <filesystem>
#include <memory>

static void initPinyin(QApplication &app) {
    std::filesystem::path dictPath =
#ifdef Q_OS_MAC
        std::filesystem::path(app.applicationDirPath().toStdString()) / ".." / "Resources" / "dict";
#else
        std::filesystem::path(app.applicationDirPath().toStdWString()) / "dict";
#endif
    Pinyin::setDictionaryPath(dictPath.make_preferred().string());

    if (!dstools::ServiceLocator::get<dstools::IG2PProvider>()) {
        auto *g2p = new dstools::PinyinG2PProvider;
        dstools::ServiceLocator::set<dstools::IG2PProvider>(g2p);
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DsLabeler");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::registerPostInitHook(&initPinyin);
    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    dstools::registerDomainFormatAdapters();
    dsfw::Theme::instance().init(app);

    auto *modelManager = dstools::ServiceLocator::get<dstools::IModelManager>();
    if (modelManager)
        dstools::registerModelProviders(*modelManager);

    static constexpr int kDefaultWidth = 1400;
    static constexpr int kDefaultHeight = 900;

    dsfw::AppShell shell;
    shell.setWindowTitle(QStringLiteral("DsLabeler — DiffSinger Dataset Labeler"));
    shell.resize(kDefaultWidth, kDefaultHeight);

    // Shared data source (lifetime = app)
    auto *dataSource = new dstools::ProjectDataSource(&shell);
    auto *settingsBackend = new dstools::AppSettingsBackend(&shell);
    std::unique_ptr<dstools::DsProject> project;

    // ── Register all 7 pages ─────────────────────────────────────────────

    // Page 0: 工程 (Welcome)
    auto *welcomePage = new dstools::WelcomePage(&shell);
    shell.addPage(welcomePage, "welcome", {}, QStringLiteral("工程"));

    // Page 1: 切片 (Slicer)
    auto *slicerPage = new dstools::DsSlicerPage(&shell);
    slicerPage->setDataSource(dataSource);
    shell.addPage(slicerPage, "slicer", {}, QStringLiteral("切片"));

    // Page 2: 歌词 (MinLabel)
    auto *minLabelPage = new dstools::MinLabelPage(&shell);
    minLabelPage->setDataSource(dataSource, settingsBackend);
    shell.addPage(minLabelPage, "minlabel", {}, QStringLiteral("歌词"));

    // Page 3: 音素 (PhonemeLabeler)
    auto *phonemePage = new dstools::PhonemeLabelerPage(&shell);
    phonemePage->setDataSource(dataSource, settingsBackend);
    shell.addPage(phonemePage, "phoneme", {}, QStringLiteral("音素"));

    // Page 4: 音高 (PitchLabeler)
    auto *pitchPage = new dstools::PitchLabelerPage(&shell);
    pitchPage->setDataSource(dataSource, settingsBackend);
    shell.addPage(pitchPage, "pitch", {}, QStringLiteral("音高"));

    // Page 5: 导出 (Export)
    auto *exportPage = new dstools::ExportPage(&shell);
    exportPage->setDataSource(dataSource);
    shell.addPage(exportPage, "export", {}, QStringLiteral("导出"));

    // Page 6: 设置 (Settings) — moved to last per ADR-64
    auto *settingsPage = new dstools::SettingsPage(settingsBackend, &shell);
    shell.addPage(settingsPage, "settings", {}, QStringLiteral("设置"));

    if (modelManager) {
        QObject::connect(settingsPage, &dstools::SettingsPage::modelReloadRequested,
                         modelManager, &dstools::IModelManager::invalidateModel);
    }

    // ── Project lifecycle ────────────────────────────────────────────────

    auto loadProject = [&](std::unique_ptr<dstools::DsProject> proj, const QString &path) {
        project = std::move(proj);

        const QString workDir = project->workingDirectory().isEmpty()
                                    ? QFileInfo(path).absolutePath()
                                    : project->workingDirectory();

        dataSource->setProject(project.get(), workDir);

        shell.setWindowTitle(
            QStringLiteral("DsLabeler — %1").arg(QFileInfo(path).fileName()));

        // Switch to slicer page
        shell.setCurrentPage(1);
    };

    QObject::connect(welcomePage, &dstools::WelcomePage::projectLoaded,
                     &shell, [&](dstools::DsProject *proj, const QString &path) {
        loadProject(std::unique_ptr<dstools::DsProject>(proj), path);
    });

    // Save on quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (project) {
            settingsPage->applySettings();
            QString error;
            project->save(error);
        }
        project.reset();
    });

    // ── Page-switch guard: check for dsitems when entering downstream pages ──
    QObject::connect(&shell, &dsfw::AppShell::currentPageChanged, &shell,
                     [&](int index) {
        // Pages 2-4 (minlabel, phoneme, pitch) need items from slicer
        if (index < 2 || index > 4)
            return;
        if (!project || !dataSource)
            return;

        const auto &items = project->items();
        if (!items.empty())
            return; // items exist, all good

        // Check if slicer has slice points
        // (slicerPage stores per-file slice points internally)
        auto ret = QMessageBox::question(
            &shell,
            QStringLiteral("尚无切片数据"),
            QStringLiteral("当前工程没有切片条目 (dsitem)。\n"
                           "是否按照切片页中的切点自动生成切片？\n\n"
                           "选择「否」将返回切片页。"),
            QMessageBox::Yes | QMessageBox::No);

        if (ret == QMessageBox::Yes) {
            // Trigger export from slicer page to create items
            shell.setCurrentPage(1); // go to slicer
            // The user should use the export function on the slicer page
        } else {
            shell.setCurrentPage(1); // go back to slicer
        }
    });

    shell.show();

    // Handle command-line .dsproj argument
    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive)) {
            QString error;
            auto proj = std::make_unique<dstools::DsProject>(dstools::DsProject::load(arg, error));
            if (error.isEmpty()) {
                loadProject(std::move(proj), arg);
            }
        }
    }

    return app.exec();
}
