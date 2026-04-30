#include <QApplication>
#include <dstools/AppInit.h>
#include <dsfw/FramelessHelper.h>
#include <dsfw/Theme.h>
#include "gui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GameInfer");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    MainWindow window;
    dsfw::FramelessHelper::apply(&window);
    window.show();
    return app.exec();
}
