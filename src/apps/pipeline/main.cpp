#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/FramelessHelper.h>
#include <dstools/Theme.h>
#include <dstools/PinyinG2PProvider.h>
#include <dsfw/ServiceLocator.h>
#include <cpp-pinyin/G2pglobal.h>
#include "PipelineWindow.h"

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
    dstools::Theme::instance().init(app);

    PipelineWindow window;
    dstools::FramelessHelper::apply(&window);
    window.show();
    return app.exec();
}
