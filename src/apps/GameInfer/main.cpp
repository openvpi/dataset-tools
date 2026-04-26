#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "gui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GameInfer");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    MainWindow window;
    window.show();
    return app.exec();
}
