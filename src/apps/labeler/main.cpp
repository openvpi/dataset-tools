#include <QApplication>
#include <dstools/AppInit.h>
#include <dsfw/Theme.h>

#include "LabelerWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DiffSinger Labeler");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    static constexpr int kDefaultWidth = 1400;
    static constexpr int kDefaultHeight = 900;

    dstools::labeler::LabelerWindow window;
    window.resize(kDefaultWidth, kDefaultHeight);
    window.show();

    // Handle command-line .dsproj argument
    if (argc > 1) {
        QString arg = QString::fromLocal8Bit(argv[1]);
        if (arg.endsWith(QLatin1String(".dsproj"), Qt::CaseInsensitive)) {
            window.loadProject(arg);
        }
    }

    return app.exec();
}
