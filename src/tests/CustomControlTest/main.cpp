#include "QDebug"
#include <QApplication>
#include <QSlider>
#include <QVBoxLayout>

#include "EditLabel.h"
#include "MainWindow.h"
#include "ProgressIndicator.h"
#include "SeekBar.h"
#include "ShadowButton.h"
#include "SwitchButton.h"
#include "ToolTip.h"

#ifdef Q_OS_WIN
#include <dwmapi.h>
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    MainWindow w;
#ifdef Q_OS_WIN
    // make window transparent
    w.setStyleSheet(QString("QMainWindow { background: transparent }"));

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

    // Enable Mica background
    auto backDropType = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_SYSTEMBACKDROP_TYPE, &backDropType,
                          sizeof(backDropType));

    // Extend title bar blur effect into client area
    constexpr int mgn = -1; // -1 means "sheet of glass" effect
    MARGINS margins = {mgn, mgn, mgn, mgn};
    DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(w.winId()), &margins);

    // Light theme
    uint dark = 0;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_IMMERSIVE_DARK_MODE, &dark,
                          sizeof(dark));
#endif

    auto mainWidget = new QWidget;

    auto editLabel1 = new EditLabel;

    auto editLabel2 = new EditLabel("Track 1");

    auto progressBar1 = new ProgressIndicator;
    progressBar1->setSecondaryValue(75);
    progressBar1->setValue(50);
    progressBar1->setCurrentTaskValue(25);

    auto progressBar2 = new ProgressIndicator;
    progressBar2->setIndeterminate(true);

    auto progressBar3 = new ProgressIndicator;
    progressBar3->setValue(50);
    progressBar3->setSecondaryValue(75);
    progressBar3->setCurrentTaskValue(25);
    progressBar3->setTaskStatus(ProgressIndicator::Warning);

    auto progressBar4 = new ProgressIndicator;
    progressBar4->setIndeterminate(true);
    progressBar4->setTaskStatus(ProgressIndicator::Warning);

    auto progressBar5 = new ProgressIndicator;
    progressBar5->setValue(50);
    progressBar5->setSecondaryValue(75);
    progressBar5->setCurrentTaskValue(25);
    progressBar5->setTaskStatus(ProgressIndicator::Error);

    auto progressBar6 = new ProgressIndicator;
    progressBar6->setIndeterminate(true);
    progressBar6->setTaskStatus(ProgressIndicator::Error);

    auto progressRing1 = new ProgressIndicator(ProgressIndicator::Ring);
    progressRing1->setValue(50);
    progressRing1->setSecondaryValue(75);
    progressRing1->setCurrentTaskValue(25);

    auto progressRing2 = new ProgressIndicator(ProgressIndicator::Ring);
    progressRing2->setTaskStatus(ProgressIndicator::Warning);
    progressRing2->setValue(50);
    progressRing2->setSecondaryValue(75);
    progressRing2->setCurrentTaskValue(25);

    auto progressRing3 = new ProgressIndicator(ProgressIndicator::Ring);
    progressRing3->setTaskStatus(ProgressIndicator::Error);
    progressRing3->setValue(50);
    progressRing3->setSecondaryValue(75);
    progressRing3->setCurrentTaskValue(25);

    auto progressRingNormalLayout = new QHBoxLayout;
    progressRingNormalLayout->addWidget(progressRing1);
    progressRingNormalLayout->addWidget(progressRing2);
    progressRingNormalLayout->addWidget(progressRing3);

    auto progressRing4 = new ProgressIndicator(ProgressIndicator::Ring);
    progressRing4->setIndeterminate(true);

    auto progressRing5 = new ProgressIndicator(ProgressIndicator::Ring);
    progressRing5->setTaskStatus(ProgressIndicator::Warning);
    progressRing5->setIndeterminate(true);

    auto progressRing6 = new ProgressIndicator(ProgressIndicator::Ring);
    progressRing6->setTaskStatus(ProgressIndicator::Error);
    progressRing6->setIndeterminate(true);

    auto progressRingIndeterminateLayout = new QHBoxLayout;
    progressRingIndeterminateLayout->addWidget(progressRing4);
    progressRingIndeterminateLayout->addWidget(progressRing5);
    progressRingIndeterminateLayout->addWidget(progressRing6);

    auto sliderValue = new SeekBar;
    sliderValue->setMax(100);
    sliderValue->setMin(0);
    sliderValue->setValue(50);
    QObject::connect(sliderValue, &SeekBar::valueChanged, &w, [=](int value) {
        progressBar1->setValue(value);
        progressBar3->setValue(value);
        progressBar5->setValue(value);
        progressRing1->setValue(value);
        progressRing2->setValue(value);
        progressRing3->setValue(value);
    });

    auto sliderSecondaryValue = new SeekBar;
    sliderSecondaryValue->setMax(100);
    sliderSecondaryValue->setMin(0);
    sliderSecondaryValue->setValue(75);
    sliderSecondaryValue->setStyleSheet(QString("qproperty-trackActiveColor: rgb(159, 189, 255);"));
    QObject::connect(sliderSecondaryValue, &SeekBar::valueChanged, &w, [=](int value) {
        progressBar1->setSecondaryValue(value);
        progressBar3->setSecondaryValue(value);
        progressBar5->setSecondaryValue(value);
        progressRing1->setSecondaryValue(value);
        progressRing2->setSecondaryValue(value);
        progressRing3->setSecondaryValue(value);
    });

    auto sliderCurrentTaskValue = new SeekBar;
    sliderCurrentTaskValue->setMax(100);
    sliderCurrentTaskValue->setMin(0);
    sliderCurrentTaskValue->setValue(25);
    sliderCurrentTaskValue->setStyleSheet(QString("qproperty-trackActiveColor: rgb(113, 218, 255); "
                                                  "qproperty-trackInactiveColor: rgb(112, 156, 255); "));
    QObject::connect(sliderCurrentTaskValue, &SeekBar::valueChanged, &w, [=](int value) {
        progressBar1->setCurrentTaskValue(value);
        progressBar3->setCurrentTaskValue(value);
        progressBar5->setCurrentTaskValue(value);
        progressRing1->setCurrentTaskValue(value);
        progressRing2->setCurrentTaskValue(value);
        progressRing3->setCurrentTaskValue(value);
    });

    auto shadowButton = new ShadowButton(mainWidget);
    shadowButton->setText("ShadowButton");
    shadowButton->setStyleSheet(QString("border: 1px solid #d4d4d4; background-color: #fff; "
                                        "border-radius: 6px; color: #333; padding: 6px 12px;"));

    auto switchButton1 = new SwitchButton;
    auto switchButton2 = new SwitchButton;
    switchButton2->setValue(true);
    auto switchButtonLayout = new QHBoxLayout;
    switchButtonLayout->addWidget(switchButton1);
    switchButtonLayout->addWidget(switchButton2);

    auto btnToolTip1 = new QPushButton;
    btnToolTip1->setText("ToolTip 1");
    btnToolTip1->setToolTip("meow~");
    btnToolTip1->installEventFilter(new ToolTipFilter(btnToolTip1));

    auto btnToolTip2 = new QPushButton;
    btnToolTip2->setText("ToolTip 2");
//    btnToolTip2->setToolTip("灯火阑珊");
    btnToolTip2->setToolTip("运行“DiffScope”");
    auto filter2 = new ToolTipFilter(btnToolTip2, 0, true);
    filter2->setShortcutKey("Shift + F10");
//    filter2->setMessage(QList<QString> {
//        "曾在梦里 诗里 歌里 风里寻",
//        "循着你的脚步",
//        "采撷一字 一句 一丝 一缕心",
//        "心上人的眷顾",
//        "曾在梦里 诗里 风里寻",
//        "众里寻他千百度",
//        "蓦然回首 那人却在 灯火阑珊处"
//    });
    btnToolTip2->installEventFilter(filter2);

    auto btnToolTip3 = new QPushButton;
    btnToolTip3->setText("ToolTip 3");
    btnToolTip3->setToolTip("萨日朗！！！");
    auto filter3 = new ToolTipFilter(btnToolTip3, 0, true, false);
    filter3->setShowDelay(0);
    filter3->setFollowCursor(true);
    filter3->setAnimation(false);
    filter3->setShortcutKey("Ctrl + S");
    filter3->setMessage(QList<QString> {
        "你这瓜保熟吗？",
        "你是故意找茬？",
        "我开水果摊能卖你生瓜？",
        "这称有问题啊",
        "你tm劈我瓜！",
        "萨日朗~ 诶华强萨日朗~"
    });
    btnToolTip3->installEventFilter(filter3);

    auto toolTipButtonLayout = new QHBoxLayout;
    toolTipButtonLayout->addWidget(btnToolTip1);
    toolTipButtonLayout->addWidget(btnToolTip2);
    toolTipButtonLayout->addWidget(btnToolTip3);

    auto verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(editLabel1);
    mainLayout->addWidget(editLabel2);
    mainLayout->addWidget(progressBar1);
    mainLayout->addWidget(progressBar2);
    mainLayout->addWidget(progressBar3);
    mainLayout->addWidget(progressBar4);
    mainLayout->addWidget(progressBar5);
    mainLayout->addWidget(progressBar6);
    mainLayout->addLayout(progressRingNormalLayout);
    mainLayout->addLayout(progressRingIndeterminateLayout);
    mainLayout->addWidget(sliderValue);
    mainLayout->addWidget(sliderSecondaryValue);
    mainLayout->addWidget(sliderCurrentTaskValue);
    mainLayout->addLayout(toolTipButtonLayout);
    mainLayout->addLayout(switchButtonLayout);
    mainLayout->addWidget(shadowButton);
//    mainLayout->addItem(verticalSpacer);

    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(300, 600);
    w.show();

    return QApplication::exec();
}