#include "gui/MainWindow.h"

#include <QApplication>

#include <dstools/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("DiffSinger Pitch Labeler");
    app.setOrganizationName("DiffSinger");
    app.setApplicationVersion("0.1.0");

    // Initialize theme system
    dstools::Theme::instance().init(app);

    dstools::pitchlabeler::MainWindow window;
    window.show();

    return app.exec();
}