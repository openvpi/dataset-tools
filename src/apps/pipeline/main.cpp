#include <QApplication>
#include <dstools/AppInit.h>
#include <dsfw/AppShell.h>
#include <dsfw/AppSettings.h>
#include <dsfw/Theme.h>
#include <dstools/PinyinG2PProvider.h>
#include <dsfw/ServiceLocator.h>
#include <cpp-pinyin/G2pglobal.h>
#include "PipelinePage.h"

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
    app.setApplicationName("Dataset Pipeline");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::registerPostInitHook(&initPinyin);
    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    dstools::AppSettings settings("DatasetPipeline");
    auto *page = new PipelinePage(&settings, &shell);
    shell.addPage(page, "pipeline", {}, "Pipeline");
    shell.setWindowTitle(page->windowTitle());
    shell.resize(1200, 800);
    shell.show();
    return app.exec();
}
