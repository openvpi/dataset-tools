#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "PipelineWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Dataset Pipeline");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app, /*initPinyin=*/true);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    PipelineWindow window;
    window.show();
    return app.exec();
}
