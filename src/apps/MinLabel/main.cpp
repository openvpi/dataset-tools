#include <QApplication>
#include <dstools/AppInit.h>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>
#include <dstools/PinyinG2PProvider.h>
#include <dsfw/ServiceLocator.h>
#include <cpp-pinyin/G2pglobal.h>
#include "gui/MinLabelPage.h"

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
    app.setApplicationName("MinLabel");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::registerPostInitHook(&initPinyin);
    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    auto *page = new Minlabel::MinLabelPage(&shell);
    shell.addPage(page, "min-label", {}, "MinLabel");
    shell.resize(1280, 720);
    shell.show();
    return app.exec();
}
