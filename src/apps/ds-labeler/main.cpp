#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

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

#include <cmath>
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

    // ── Page-switch guard: check slice consistency when entering downstream pages ──
    QObject::connect(&shell, &dsfw::AppShell::currentPageChanged, &shell,
                     [&](int index) {
        // Pages 2-4 (minlabel, phoneme, pitch) need items from slicer
        if (index < 2 || index > 4)
            return;
        if (!project || !dataSource)
            return;

        const auto &items = project->items();
        const auto &slicerState = project->slicerState();

        // Case 1: No items exported at all but slicer has audio files with slice points
        if (items.empty()) {
            if (!slicerState.audioFiles.isEmpty()) {
                auto ret = QMessageBox::question(
                    &shell,
                    QStringLiteral("未导出切片"),
                    QStringLiteral("切片页面已有音频文件和切点，但尚未导出切片。\n"
                                   "是否回到切片页面导出？"),
                    QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes)
                    QTimer::singleShot(0, &shell, [&shell]() { shell.setCurrentPage(1); });
            } else {
                auto ret = QMessageBox::question(
                    &shell,
                    QStringLiteral("未切片"),
                    QStringLiteral("尚未导出切片，是否回到切片页面？"),
                    QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes)
                    QTimer::singleShot(0, &shell, [&shell]() { shell.setCurrentPage(1); });
            }
            return;
        }

        // Case 2: Check consistency between slicer state and exported items.
        // SlicerState.slicePoints keys are original audio paths;
        // items have exported WAV paths. Match by basename prefix.
        QStringList problemFiles;

        for (const auto &[originalPath, points] : slicerState.slicePoints) {
            if (points.empty())
                continue;

            QString baseName = QFileInfo(originalPath).completeBaseName();
            int expectedSegments = static_cast<int>(points.size()) + 1;

            // Count project items whose ID starts with this audio's basename
            int actualSegments = 0;
            for (const auto &item : items) {
                if (item.id.startsWith(baseName))
                    ++actualSegments;
            }

            if (actualSegments == 0) {
                // This audio file was sliced but never exported
                problemFiles.append(QFileInfo(originalPath).fileName()
                                    + QStringLiteral(" (未导出)"));
            } else if (actualSegments != expectedSegments) {
                // Slice count mismatch — points changed after export
                problemFiles.append(QFileInfo(originalPath).fileName()
                                    + QStringLiteral(" (切点数不一致: 期望 %1 段, 实际 %2 段)")
                                          .arg(expectedSegments).arg(actualSegments));
            } else {
                // Slice count matches — verify boundary positions (tolerance: 1ms)
                constexpr double kToleranceUs = 1000.0;
                bool mismatch = false;
                int segIdx = 0;
                for (const auto &item : items) {
                    if (!item.id.startsWith(baseName))
                        continue;
                    if (item.slices.empty())
                        continue;
                    const auto &slice = item.slices[0];
                    double expectedStart = (segIdx == 0) ? 0.0 : points[segIdx - 1] * 1e6;
                    if (std::abs(expectedStart - static_cast<double>(slice.inPos)) > kToleranceUs) {
                        mismatch = true;
                        break;
                    }
                    ++segIdx;
                }
                if (mismatch)
                    problemFiles.append(QFileInfo(originalPath).fileName()
                                        + QStringLiteral(" (切点位置已变化)"));
            }
        }

        // Also check for audio files in slicer list that have no slice points at all
        for (const auto &audioFile : slicerState.audioFiles) {
            QString resolved = audioFile;
            if (QDir::isRelativePath(resolved))
                resolved = QDir(project->workingDirectory()).absoluteFilePath(resolved);
            if (slicerState.slicePoints.find(resolved) == slicerState.slicePoints.end()) {
                // Audio file added but never sliced
                problemFiles.append(QFileInfo(audioFile).fileName()
                                    + QStringLiteral(" (未切片)"));
            }
        }

        if (problemFiles.isEmpty())
            return;

        // Show warning dialog
        QMessageBox msgBox(&shell);
        msgBox.setWindowTitle(QStringLiteral("切片状态不一致"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(QStringLiteral("以下音频文件的切片结果与当前切点不一致，\n"
                                      "继续操作可能导致标注数据错误："));
        msgBox.setDetailedText(problemFiles.join(QStringLiteral("\n")));
        auto *goBackBtn = msgBox.addButton(QStringLiteral("回到切片页面重新导出"), QMessageBox::AcceptRole);
        msgBox.addButton(QStringLiteral("忽略继续"), QMessageBox::RejectRole);

        msgBox.exec();
        if (msgBox.clickedButton() == goBackBtn) {
            QTimer::singleShot(0, &shell, [&shell]() { shell.setCurrentPage(1); });
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
