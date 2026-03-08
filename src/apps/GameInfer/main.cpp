#include "gui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application information
    app.setApplicationName("GameInfer");
    app.setApplicationVersion("0.0.0.1");
    app.setOrganizationName("GameInfer Team");

    MainWindow window;
    window.show();

    return app.exec();
}