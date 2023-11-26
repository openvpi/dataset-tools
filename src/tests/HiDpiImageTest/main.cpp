//
// Created by fluty on 2023/11/27.
//


#include "ImageView.h"


#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication a(argc, argv);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    QMainWindow w;

    auto lbImage = new QLabel;
    lbImage->setMinimumSize(1200, 675);
    auto pix = QPixmap("D:/图片/桌面壁纸/dev-build/01-Wave_DM-4k.png");
    lbImage->setPixmap(pix.scaled(lbImage->width(), pix.scaledToWidth(lbImage->width()).height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto imageView = new ImageView;
    imageView->setImage(pix);

    auto mainLayout = new QVBoxLayout;

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    // w.setCentralWidget(mainWidget);
    // w.setCentralWidget(lbImage);
    w.setCentralWidget(imageView);
    w.resize(1200, 675);
    w.show();

    return QApplication::exec();
}