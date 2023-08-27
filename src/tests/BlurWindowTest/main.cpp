//
// Created by fluty on 2023/8/27.
//

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>

#ifdef Q_OS_WIN
#    include <dwmapi.h>
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    QApplication::setFont(QFont("Microsoft Yahei UI", 9));

    QMainWindow w;

    auto mainLayout = new QVBoxLayout;

#ifdef Q_OS_WIN
    // make window transparent
    w.setStyleSheet(QString("background: transparent"));

    enum DWM_SYSTEMBACKDROP_TYPE : uint
    {
        DWMSBT_AUTO = 0,

        /// <summary>
        /// no backdrop
        /// </summary>
        DWMSBT_NONE = 1,

        /// <summary>
        /// Use tinted blurred wallpaper backdrop (Mica)
        /// </summary>
        DWMSBT_MAINWINDOW = 2,

        /// <summary>
        /// Use Acrylic backdrop
        /// </summary>
        DWMSBT_TRANSIENTWINDOW = 3,

        /// <summary>
        /// Use blurred wallpaper backdrop
        /// </summary>
        DWMSBT_TABBEDWINDOW = 4
    };
    uint DWMWA_SYSTEMBACKDROP_TYPE = 38;
    uint DWMWA_IMMERSIVE_DARK_MODE = 20;

    DWM_SYSTEMBACKDROP_TYPE backdropType = DWMSBT_TRANSIENTWINDOW;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                          sizeof(backdropType));
    // Extend title bar blur effect into client area
    constexpr int mgn = -1; // -1 means "sheet of glass" effect
    MARGINS margins = {mgn, mgn, mgn, mgn};
    DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(w.winId()), &margins);

    auto qss = QString("border: 1px solid #a0d4d4d4; background-color: #a0ffffff; "
                       "border-radius: 6px; color: #333; padding: 6px 12px;");

    auto btnSwitchColor = new QPushButton;
    btnSwitchColor->setText("Switch Light/Dark");
    btnSwitchColor->setStyleSheet(qss);
    btnSwitchColor->setCheckable(true);
    btnSwitchColor->setChecked(false);
    QObject::connect(btnSwitchColor, &QPushButton::toggled, &w, [&](bool) {
//        btnSwitchColor->setChecked(!btnSwitchColor->isChecked());
        uint dark = btnSwitchColor->isChecked() ? 1 : 0;
        qDebug() << dark;
        DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_IMMERSIVE_DARK_MODE, &dark,
                              sizeof(dark));
    });

    mainLayout->addWidget(btnSwitchColor);
#endif

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(400, 300);
    w.show();

    return QApplication::exec();
}