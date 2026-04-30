#include <QApplication>
#include <dstools/AppInit.h>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>
#include <dsfw/AppSettings.h>
#include "gui/GameInferPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GameInfer");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    dstools::AppSettings settings("GameInfer");
    auto *page = new GameInferPage(&settings, &shell);
    shell.addPage(page, "game-infer", {}, "GameInfer");
    shell.setWindowTitle("GameInfer - 音频转MIDI工具");
    shell.resize(800, 450);
    shell.show();
    return app.exec();
}
