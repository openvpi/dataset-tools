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

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    QMainWindow w;

    auto mainLayout = new QVBoxLayout;

#ifdef Q_OS_WIN
    // make window transparent
    w.setStyleSheet(QString("background: transparent"));

    enum DWM_SYSTEMBACKDROP_TYPE : uint
    {
        DWMSBT_AUTO = 0,

        /// no backdrop
        DWMSBT_NONE = 1,

        /// Mica
        DWMSBT_MAINWINDOW = 2,

        /// Acrylic
        DWMSBT_TRANSIENTWINDOW = 3,

        /// Tabbed Mica
        DWMSBT_TABBEDWINDOW = 4
    };
    uint DWMWA_SYSTEMBACKDROP_TYPE = 38;
    uint DWMWA_IMMERSIVE_DARK_MODE = 20;

    auto backdropType = DWMSBT_TRANSIENTWINDOW;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_SYSTEMBACKDROP_TYPE, &backdropType,
                          sizeof(backdropType));
    // Extend title bar blur effect into client area
    constexpr int mgn = -1; // -1 means "sheet of glass" effect
    MARGINS margins = {mgn, mgn, mgn, mgn};
    DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(w.winId()), &margins);

    auto qss = QString("QPushButton { border: 1px solid #a0d4d4d4; background-color: #a0ffffff; "
                       "border-radius: 6px; color: #333; padding: 6px 12px; }"
                       "QToolTip { background-color: #f3f3f3; border-radius: 6px; "
                       "color: #333; padding: 4px 4px; font-size: 9pt }");

    auto btnSwitchColor = new QPushButton;
    btnSwitchColor->setText("Light/Dark");
//    btnSwitchColor->setToolTip("Switch theme");
    btnSwitchColor->setStyleSheet(qss);
    btnSwitchColor->setCheckable(true);
    btnSwitchColor->setChecked(false);
    QObject::connect(btnSwitchColor, &QPushButton::toggled, &w, [&](bool) {
        uint dark = btnSwitchColor->isChecked() ? 1 : 0;
        DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_IMMERSIVE_DARK_MODE, &dark,
                              sizeof(dark));
    });

    auto btnSwitchMaterial = new QPushButton;
    btnSwitchMaterial->setText("Acrylic/Mica");
    btnSwitchMaterial->setStyleSheet(qss);
    btnSwitchMaterial->setCheckable(true);
    btnSwitchMaterial->setChecked(false);
    QObject::connect(btnSwitchMaterial, &QPushButton::toggled, &w, [&](bool) {
        auto material = btnSwitchMaterial->isChecked()
                                               ? DWMSBT_MAINWINDOW
                                               : DWMSBT_TRANSIENTWINDOW;
        DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_SYSTEMBACKDROP_TYPE, &material,
                             sizeof(material));
    });

    mainLayout->addWidget(btnSwitchColor);
    mainLayout->addWidget(btnSwitchMaterial);
#endif

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(400, 300);
    w.show();

    return QApplication::exec();
}