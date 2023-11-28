//
// Created by fluty on 2023/11/27.
//

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

#include "ImageView.h"

// #ifdef Q_OS_WIN
// #    include <Windows.h>
// #    include <winuser.h>
// #endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QCoreApplication::setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false);
    // QCoreApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, true);
    QApplication a(argc, argv);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    QMainWindow w;

    w.setAttribute(Qt::WA_AcceptTouchEvents);

    auto pix = QPixmap("D:/图片/桌面壁纸/dev-build/01-Wave_DM-4k.png");

    auto imageView = new ImageView;
    imageView->setImage(pix);
    imageView->setScaleType(ImageView::CenterInside);

    auto mainLayout = new QVBoxLayout;

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    // w.setCentralWidget(mainWidget);
    // w.setCentralWidget(lbImage);
    w.setCentralWidget(imageView);
    w.resize(800, 450);
    w.show();

// #ifdef Q_OS_WIN
//     auto winId = reinterpret_cast<HWND>(w.winId());
//     RegisterTouchWindow(winId, TWF_WANTPALM);
// #endif

    return QApplication::exec();
}