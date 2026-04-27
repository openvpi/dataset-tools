#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/FramelessHelper.h>
#include <dstools/Theme.h>
#include "gui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MinLabel");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app, /*initPinyin=*/true, /*initCrashHandler=*/true))
        return 0;
    dstools::Theme::instance().init(app);

    Minlabel::MainWindow w;
    dstools::FramelessHelper::apply(&w);
    w.show();
    return app.exec();
}
