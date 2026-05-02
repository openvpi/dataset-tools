#include <QApplication>
#include <QToolBar>

#include <dsfw/AppShell.h>
#include <dsfw/ServiceLocator.h>
#include <dsfw/Theme.h>
#include <dstools/AppInit.h>
#include <dstools/PinyinG2PProvider.h>

#include <cpp-pinyin/G2pglobal.h>

#include <MinLabelPage.h>
#include <PhonemeLabelerPage.h>
#include <PitchLabelerPage.h>

#include <filesystem>

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
    app.setApplicationName("LabelSuite");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::registerPostInitHook(&initPinyin);
    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    shell.setWindowTitle(QStringLiteral("LabelSuite"));

    static constexpr int kPageMinLabel = 0;
    static constexpr int kPagePhoneme = 1;
    static constexpr int kPagePitch = 2;

    // Page 1: 歌词标注 (MinLabel) — same as standalone MinLabel exe
    auto *minLabelPage = new Minlabel::MinLabelPage(&shell);
    shell.addPage(minLabelPage, "minlabel", {}, QStringLiteral("歌词"));

    // Page 2: 音素标注 (PhonemeLabeler) — same as standalone PhonemeLabeler exe
    auto *phonemePage = new dstools::phonemelabeler::PhonemeLabelerPage(&shell);
    shell.addPage(phonemePage, "phoneme", {}, QStringLiteral("音素"));
    shell.setSettings(&phonemePage->settings());

    // Add PhonemeLabeler toolbar to shell (same as standalone), but hide when not active
    QToolBar *phonemeToolbar = phonemePage->toolbar();
    if (phonemeToolbar) {
        shell.addToolBar(phonemeToolbar);
        phonemeToolbar->setVisible(false); // hidden initially (MinLabel is first)
    }

    // Page 3: 音高标注 (PitchLabeler) — same as standalone PitchLabeler exe
    auto *pitchPage = new dstools::pitchlabeler::PitchLabelerPage(&shell);
    shell.addPage(pitchPage, "pitch", {}, QStringLiteral("音高"));

    // Toggle toolbar visibility and title on page switch
    QObject::connect(&shell, &dsfw::AppShell::currentPageChanged,
                     &shell, [&](int index) {
        // PhonemeLabeler toolbar: only visible on phoneme page
        if (phonemeToolbar)
            phonemeToolbar->setVisible(index == kPagePhoneme);

        // Update window title to match active page
        switch (index) {
            case kPageMinLabel:
                shell.setWindowTitle(minLabelPage->windowTitle());
                break;
            case kPagePhoneme:
                shell.setWindowTitle(phonemePage->windowTitle());
                break;
            case kPagePitch:
                shell.setWindowTitle(pitchPage->windowTitle());
                break;
        }
    });

    // Sync title changes from PitchLabeler (file load/save/modify)
    auto updatePitchTitle = [&]() {
        if (shell.currentPageIndex() == kPagePitch)
            shell.setWindowTitle(pitchPage->windowTitle());
    };
    QObject::connect(pitchPage, &dstools::pitchlabeler::PitchLabelerPage::modificationChanged,
                     &shell, [&](bool) { updatePitchTitle(); });
    QObject::connect(pitchPage, &dstools::pitchlabeler::PitchLabelerPage::fileLoaded,
                     &shell, [&](const QString &) { updatePitchTitle(); });
    QObject::connect(pitchPage, &dstools::pitchlabeler::PitchLabelerPage::fileSaved,
                     &shell, [&](const QString &) { updatePitchTitle(); });

    // Sync title changes from MinLabel (directory change)
    QObject::connect(minLabelPage, &Minlabel::MinLabelPage::workingDirectoryChanged,
                     &shell, [&](const QString &) {
        if (shell.currentPageIndex() == kPageMinLabel)
            shell.setWindowTitle(minLabelPage->windowTitle());
    });

    // Save config on shutdown
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        pitchPage->saveConfig();
    });

    constexpr int kDefaultWidth = 1280;
    constexpr int kDefaultHeight = 800;
    shell.resize(kDefaultWidth, kDefaultHeight);
    shell.show();

    return app.exec();
}
