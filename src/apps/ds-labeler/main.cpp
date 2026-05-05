#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

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
        if (items.empty()) {
            auto ret = QMessageBox::question(
                &shell,
                QStringLiteral("未切片"),
                QStringLiteral("尚未导出切片，是否回到切片页面？"),
                QMessageBox::Yes | QMessageBox::No);
            if (ret == QMessageBox::Yes)
                shell.setCurrentPage(1);
            return;
        }

        // Check slice point consistency: compare current slicer state
        // against stored item slice durations
        const auto &slicerState = project->slicerState();
        QStringList changedFiles;

        for (const auto &item : items) {
            if (item.slices.empty())
                continue;
            const QString &audioPath = item.audioSource;
            auto it = slicerState.slicePoints.find(audioPath);
            if (it == slicerState.slicePoints.end())
                continue;

            // Compare slice count: N slice points → N+1 segments
            const auto &points = it->second;
            if (static_cast<int>(points.size()) + 1 != static_cast<int>(item.slices.size())) {
                changedFiles.append(audioPath);
                continue;
            }

            // Compare slice boundary positions (tolerance: 1ms = 1000us)
            constexpr double kToleranceUs = 1000.0;
            bool mismatch = false;
            for (size_t s = 0; s < item.slices.size() && !mismatch; ++s) {
                double expectedStart = (s == 0) ? 0.0 : points[s - 1] * 1e6;
                double actualStart = static_cast<double>(item.slices[s].inPos);
                if (std::abs(expectedStart - actualStart) > kToleranceUs)
                    mismatch = true;
            }
            if (mismatch)
                changedFiles.append(audioPath);
        }

        if (changedFiles.isEmpty())
            return;

        // Show dialog with checkboxes for changed files
        QDialog dlg(&shell);
        dlg.setWindowTitle(QStringLiteral("切点信息变化"));
        auto *layout = new QVBoxLayout(&dlg);
        layout->addWidget(new QLabel(
            QStringLiteral("以下音频的切点已发生变化，需要重新切片导出："), &dlg));

        QList<QCheckBox *> checkboxes;
        for (const auto &file : changedFiles) {
            auto *cb = new QCheckBox(QFileInfo(file).fileName(), &dlg);
            cb->setChecked(true);
            cb->setProperty("filePath", file);
            layout->addWidget(cb);
            checkboxes.append(cb);
        }

        auto *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("回到切片页面"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("忽略继续"));
        layout->addWidget(buttons);

        QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        if (dlg.exec() == QDialog::Accepted) {
            shell.setCurrentPage(1); // Go back to slicer
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
