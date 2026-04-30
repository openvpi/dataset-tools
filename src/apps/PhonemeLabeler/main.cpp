#include "gui/MainWindow.h"

#include <QApplication>

#include <dstools/AppInit.h>
#include <dsfw/FramelessHelper.h>
#include <dsfw/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("PhonemeLabeler");
    app.setOrganizationName("DiffSinger");
    app.setApplicationVersion("0.1.0");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    dstools::phonemelabeler::MainWindow window;
    dsfw::FramelessHelper::apply(&window);
    window.show();

    // Support command-line file path
    if (argc > 1) {
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
