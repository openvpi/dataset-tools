#include "gui/MainWindow.h"

#include <QApplication>

#include <dstools/AppInit.h>
#include <dstools/FramelessHelper.h>
#include <dstools/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("PhonemeLabeler");
    app.setOrganizationName("DiffSinger");
    app.setApplicationVersion("0.1.0");

    if (!dstools::AppInit::init(app))
        return 0;
    dstools::Theme::instance().init(app);

    dstools::phonemelabeler::MainWindow window;
    dstools::FramelessHelper::apply(&window);
    window.show();

    // Support command-line file path
    if (argc > 1) {
        window.openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
