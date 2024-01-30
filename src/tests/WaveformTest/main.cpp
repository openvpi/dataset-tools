//
// Created by fluty on 2023/8/27.
//

#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include "Controller.h"

#ifdef Q_OS_WIN
#    include <dwmapi.h>
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);
    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    auto qssBase =
        "QPushButton { background: #2A2B2C; border: 1px solid #606060; "
        "border-radius: 6px; color: #F0F0F0;padding: 4px 12px;} "
        "QPushButton:hover {background-color: #343536; } "
        "QPushButton:pressed {background-color: #202122 } "
        "QScrollBar::vertical{ width:10px; background-color: transparent; border-style: none; border-radius: 4px; } "
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; } "
        "QScrollBar::handle::vertical{ border-radius:4px; width: 10px; background:rgba(255, 255, 255, 0.2); } "
        "QScrollBar::handle::vertical::hover{ background:rgba(255, 255, 255, 0.3); } "
        "QScrollBar::handle::vertical:pressed{ background:rgba(255, 255, 255, 0.1); } "
        "QScrollBar::add-line::vertical, QScrollBar::sub-line::vertical{ border:none; } "
        "QScrollBar::horizontal{ height:10px; background-color: transparent; border-style: none; border-radius: 4px; } "
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; } "
        "QScrollBar::handle::horizontal{ border-radius:4px; width: 10px; background:rgba(255, 255, 255, 0.2); } "
        "QScrollBar::handle::horizontal::hover{ background:rgba(255, 255, 255, 0.3); } "
        "QScrollBar::handle::horizontal:pressed{ background:rgba(255, 255, 255, 0.1); } "
        "QScrollBar::add-line::horizontal, QScrollBar::sub-line::horizontal{ border:none; } "
        "QSplitter { background-color: transparent; border: none; } "
        "QSplitter::handle { background-color: transparent; margin: 0px 4px; } "
        "QSplitterHandle::item:hover {} QSplitter::handle:hover { background-color: rgb(155, 186, 255); } "
        "QSplitter::handle:pressed { background-color: rgb(112, 156, 255); } QSplitter::handle:horizontal { width: "
        "4px; } "
        "QSplitter::handle:vertical { height: 6px; } "
        "QGraphicsView { border: none; background-color: #2A2B2C;}"
        "QListWidget { background: #2A2B2C; border: none } "
        "QMenu { padding: 4px; background-color: #202122; border: 1px solid #606060; "
        "border-radius: 6px; color: #F0F0F0;} "
        "QMenu::indicator { left: 6px; width: 20px; height: 20px; } QMenu::icon { left: 6px; } "
        "QMenu::item { background: transparent; color: #fff; padding: 4px 28px; } "
        "QMenu[stats=checkable]::item, QMenu[stats=icon]::item { padding-left: 12px; } "
        "QMenu::item:selected { color: #000; background-color: #9BBAFF; border: 1px solid transparent; "
        "border-style: none; border-radius: 4px; } "
        "QMenu::item:disabled { color: #d5d5d5; background-color: transparent; } "
        "QMenu::separator { height: 1.25px; background-color: #606060; margin: 6px 0; } ";
    QMainWindow w;
    w.setStyleSheet(QString("QMainWindow { background: #232425 }") + qssBase);
#ifdef Q_OS_WIN
    // make window transparent
    w.setStyleSheet(QString("QMainWindow { background: transparent }") + qssBase);
    // Enable Mica background
    auto backDropType = DWMSBT_MAINWINDOW;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_SYSTEMBACKDROP_TYPE, &backDropType,
                          sizeof(backDropType));
    // Extend title bar blur effect into client area
    constexpr int mgn = -1;
    MARGINS margins = {mgn, mgn, mgn, mgn};
    DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(w.winId()), &margins);
    // Dark theme
    uint dark = 1;
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
#endif

    auto controller = new Controller;

    auto btnNewTrack = new QPushButton;
    btnNewTrack->setText("New Track");
    QObject::connect(btnNewTrack, &QPushButton::clicked, controller, &Controller::onNewTrack);

    auto btnOpenAudioFile = new QPushButton;
    btnOpenAudioFile->setText("Add an audio file...");
    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, controller, [&]() {
        auto fileName = QFileDialog::getOpenFileName(
            btnOpenAudioFile, "Select an Audio File", ".",
            "All Audio File (*.wav *.flac *.mp3);;Wave File (*.wav);;Flac File (*.flac);;MP3 File (*.mp3)");
        if (fileName.isNull())
            return;

        controller->addAudioClipToNewTrack(fileName);
    });

    auto btnOpenProjectFile = new QPushButton;
    btnOpenProjectFile->setText("Open project...");
    QObject::connect(btnOpenProjectFile, &QPushButton::clicked, controller, [&]() {
        auto fileName =
            QFileDialog::getOpenFileName(btnOpenProjectFile, "Select a Project File", ".", "Project File (*.json)");
        if (fileName.isNull())
            return;

        controller->openProject(fileName);
    });

    auto splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(controller->tracksView());
    splitter->addWidget(controller->pianoRollView());

    auto actionButtonLayout = new QHBoxLayout;
    actionButtonLayout->addWidget(btnNewTrack);
    actionButtonLayout->addWidget(btnOpenAudioFile);
    actionButtonLayout->addWidget(btnOpenProjectFile);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(actionButtonLayout);
    mainLayout->addWidget(splitter);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1280, 720);
    w.show();

    return QApplication::exec();
}