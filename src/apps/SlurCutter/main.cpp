#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "gui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SlurCutter");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    SlurCutter::MainWindow w;
    w.show();
    return app.exec();
}
