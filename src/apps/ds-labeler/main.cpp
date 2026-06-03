#include "core/ProjectDataSource.h"
#include "dsfw/AppPaths.h"
#include "dsfw/SingleInstanceGuard.h"
#include "dsfw/Theme.h"
#include "ui/DsSlicerPage.h"
#include "ui/ExportPage.h"
#include "ui/WelcomePage.h"

#include <AppSettingsBackend.h>
#include <EnginePool.h>
#include <ModelProviderInit.h>
#include <PageDescriptors.h>
#include <PageFactory.h>
#include <SettingsPage.h>
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <cpp-pinyin/G2pglobal.h>
#include <dsfw/AppShell.h>
#include <dsfw/PathUtils.h>
#include <dsfw/ServiceLocator.h>
#include <dstools/AppInit.h>
#include <dstools/DomainInit.h>
#include <dstools/PinyinG2PProvider.h>
#include <filesystem>
#include <memory>
#include <unified-editor/AppShellConfig.h>

using namespace dstools::unified_editor;

namespace {

void checkSliceConsistency(dstools::DsProject* project, dsfw::AppShell* shell) {
    if (!project)
        return;

    const auto& items = project->items();
    const auto& slicerState = project->slicerState();

    if (items.empty()) {
        if (!slicerState.audioFiles.isEmpty()) {
            auto ret = QMessageBox::question(shell, QStringLiteral("未导出切片"),
                                             QStringLiteral("切片页面已有音频文件和切点，但尚未导出切片。\n"
                                                            "是否回到切片页面导出？"),
                                             QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes)
                QTimer::singleShot(0, shell, [shell]() { shell->setCurrentPage(1); });
        } else {
            auto ret = QMessageBox::question(shell, QStringLiteral("未切片"),
                                             QStringLiteral("尚未导出切片，是否回到切片页面？"),
                                             QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes)
                QTimer::singleShot(0, shell, [shell]() { shell->setCurrentPage(1); });
        }
        return;
    }

    auto consistencyResult = project->validateSliceConsistency();
    if (consistencyResult.ok())
        return;

    const QString issueText = QString::fromStdString(consistencyResult.error());

    QMessageBox msgBox(shell);
    msgBox.setWindowTitle(QStringLiteral("切片状态不一致"));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(QStringLiteral("以下音频文件的切片结果与当前切点不一致，\n"
                                  "继续操作可能导致标注数据错误："));
    msgBox.setDetailedText(issueText);
    auto* goBackBtn = msgBox.addButton(QStringLiteral("回到切片页面重新导出"), QMessageBox::AcceptRole);
    msgBox.addButton(QStringLiteral("忽略继续"), QMessageBox::RejectRole);

    msgBox.exec();
    if (msgBox.clickedButton() == goBackBtn) {
        QTimer::singleShot(0, shell, [shell]() { shell->setCurrentPage(1); });
    }
}

} // namespace

static void initPinyin(QApplication& app) {
    std::filesystem::path dictPath =
#ifdef Q_OS_MAC
        dsfw::PathUtils::toStdPath(app.applicationDirPath()) / ".." / "Resources" / "dict";
#else
        dsfw::PathUtils::toStdPath(app.applicationDirPath()) / "dict";
#endif
    Pinyin::setDictionaryPath(dsfw::PathUtils::toUtf8(dictPath.make_preferred()));

    if (!dstools::ServiceLocator::get<dstools::IG2PProvider>()) {
        dstools::ServiceLocator::set<dstools::IG2PProvider>(&dstools::PinyinG2PProvider::instance());
    }
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DsLabeler");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    dsfw::SingleInstanceGuard instanceGuard("DsLabeler");
    if (!instanceGuard.tryLock()) {
        instanceGuard.sendArguments(app.arguments());
        return 0;
    }

    dstools::AppInit::registerPostInitHook(&initPinyin);
    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    instanceGuard.listen();
    dstools::registerDomainFormatAdapters();
    dsfw::Theme::instance().init(app);

    auto* modelManager = &dstools::ModelManager::instance();
    if (modelManager)
        dstools::registerModelProviders(*modelManager);

    dsfw::AppShell shell;
    setupShell(shell, QStringLiteral("DsLabeler — DiffSinger Dataset Labeler"));

    showCrashRecoveryDialog(&shell);

    // Shared data source (lifetime = app)
    auto* dataSource = new dstools::ProjectDataSource(&shell);
    auto* settingsBackend = new dstools::AppSettingsBackend(&shell);
    std::unique_ptr<dstools::DsProject> project;

    // ── Register all 7 pages ─────────────────────────────────────────────

    // Page 0: 工程 (Welcome)
    auto* welcomePage = new dstools::WelcomePage(&shell);
    shell.addPage(welcomePage, "welcome", {}, QStringLiteral("工程"));

    // Page 1: 切片 (Slicer)
    auto* slicerPage = new dstools::DsSlicerPage(&shell);
    slicerPage->setDataSource(dataSource);
    shell.addPage(slicerPage, "slicer", {}, QStringLiteral("切片"));

    // Pages 2-4: 标注 (shared editor pages)
    static const dstools::MinLabelPageDescriptor minLabelDesc;
    static const dstools::PhonemeLabelerPageDescriptor phonemeDesc;
    static const dstools::PitchLabelerPageDescriptor pitchDesc;
    dstools::PageFactory::registerPages(&shell, dataSource, settingsBackend, {&minLabelDesc, &phonemeDesc, &pitchDesc});

    // Page 5: 导出 (Export)
    auto* exportPage = new dstools::ExportPage(&shell);
    exportPage->setDataSource(dataSource);
    if (modelManager) {
        auto enginePool = std::make_unique<dstools::EnginePool>(modelManager, settingsBackend, exportPage);
        exportPage->setEnginePool(std::move(enginePool));
    }
    shell.addPage(exportPage, "export", {}, QStringLiteral("导出"));

    // Pages 6-7: 设置 (Settings + Log)
    static const dstools::SettingsPageDescriptor settingsPageDesc;
    static const dstools::LogPageDescriptor logPageDesc;
    dstools::PageFactory::registerPages(&shell, nullptr, settingsBackend, {&settingsPageDesc, &logPageDesc});

    auto *settingsPage = shell.findChild<dstools::SettingsPage *>(QString(), Qt::FindDirectChildrenOnly);
    connectModelReload<dstools::SettingsPage>(&shell, modelManager);

    // ── Project lifecycle ────────────────────────────────────────────────

    auto loadProject = [&](std::unique_ptr<dstools::DsProject> proj, const QString& path) {
        project = std::move(proj);

        const QString workDir =
            project->workingDirectory().isEmpty() ? QFileInfo(path).absolutePath() : project->workingDirectory();

        dataSource->setProject(project.get(), workDir);

        shell.setWindowTitle(QStringLiteral("DsLabeler — %1").arg(QFileInfo(path).fileName()));

        // Switch to slicer page
        shell.setCurrentPage(1);
    };

    QObject::connect(welcomePage, &dstools::WelcomePage::projectLoaded, &shell,
                     [&](dstools::DsProject* proj, const QString& path) {
                         loadProject(std::unique_ptr<dstools::DsProject>(proj), path);
                     });

    // Save on quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        if (project) {
            settingsPage->applySettings();
            project->saveFile();
        }
        project.reset();
    });

    // ── Page-switch guard: check slice consistency when entering downstream pages ──
    QObject::connect(&shell, &dsfw::AppShell::currentPageChanged, &shell, [&](int index) {
        if (index < 2 || index > 4)
            return;
        if (!project || !dataSource)
            return;
        checkSliceConsistency(project.get(), &shell);
    });

    shell.show();

    QObject::connect(&instanceGuard, &dsfw::SingleInstanceGuard::argumentsReceived, &shell,
                     [&](const QStringList& args) {
                         if (args.size() > 1) {
                             QString arg = args.at(1);
                             if (arg.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive)) {
                                 auto result = dstools::DsProject::loadFile(arg);
                                 if (result.ok()) {
                                     auto proj = std::make_unique<dstools::DsProject>(std::move(result.value()));
                                     loadProject(std::move(proj), arg);
                                 }
                             }
                         }
                     });

    // Handle command-line .dsproj argument
    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive)) {
            auto result = dstools::DsProject::loadFile(arg);
            if (result.ok()) {
                auto proj = std::make_unique<dstools::DsProject>(std::move(result.value()));
                loadProject(std::move(proj), arg);
            }
        }
    }

    return app.exec();
}
