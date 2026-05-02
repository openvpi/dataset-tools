#include <QApplication>

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

    // Page 1: 歌词标注 (MinLabel)
    auto *minLabelPage = new Minlabel::MinLabelPage(&shell);
    shell.addPage(minLabelPage, "minlabel", {}, QStringLiteral("歌词"));

    // Page 2: 音素标注 (PhonemeLabeler)
    auto *phonemePage = new dstools::phonemelabeler::PhonemeLabelerPage(&shell);
    shell.addPage(phonemePage, "phoneme", {}, QStringLiteral("音素"));
    if (auto *tb = phonemePage->toolbar())
        shell.addToolBar(tb);

    // Page 3: 音高标注 (PitchLabeler)
    auto *pitchPage = new dstools::pitchlabeler::PitchLabelerPage(&shell);
    shell.addPage(pitchPage, "pitch", {}, QStringLiteral("音高"));

    // Sync pitch page title changes
    auto updateTitle = [&]() { shell.setWindowTitle(pitchPage->windowTitle()); };
    QObject::connect(pitchPage, &dstools::pitchlabeler::PitchLabelerPage::modificationChanged,
                     &shell, [&](bool) { updateTitle(); });
    QObject::connect(pitchPage, &dstools::pitchlabeler::PitchLabelerPage::fileLoaded,
                     &shell, [&](const QString &) { updateTitle(); });
    QObject::connect(pitchPage, &dstools::pitchlabeler::PitchLabelerPage::fileSaved,
                     &shell, [&](const QString &) { updateTitle(); });

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
