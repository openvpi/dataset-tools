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
#include "Controls/PianoRoll/NoteGraphicsItem.h"
#include "Controls/PianoRoll/PianoRollBackgroundGraphicsItem.h"
#include "Controls/PianoRoll/PianoRollGraphicsScene.h"
#include "Controls/PianoRoll/PianoRollGraphicsView.h"
#include "Controls/PianoRoll/PitchEditorGraphicsItem.h"
#include "Controls/TracksEditor/TracksGraphicsView.h"

#ifdef Q_OS_WIN
#include <dwmapi.h>
#endif

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    QMainWindow w;
    w.setStyleSheet(QString("QMainWindow { background: #232425 }"));
#ifdef Q_OS_WIN
    // make window transparent
    w.setStyleSheet(QString("QMainWindow { background: transparent }"));
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
    DwmSetWindowAttribute(reinterpret_cast<HWND>(w.winId()), DWMWA_USE_IMMERSIVE_DARK_MODE, &dark,
                          sizeof(dark));
#endif

    auto btnOpenAudioFile = new QPushButton;
    btnOpenAudioFile->setText("Add...");

    auto controller = new Controller;
    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, controller, [&]() {
        auto fileName = QFileDialog::getOpenFileName(
            btnOpenAudioFile, "Select an Audio File", ".",
            "All Audio File (*.wav *.flac *.mp3);;Wave File (*.wav);;Flac File (*.flac);;MP3 File (*.mp3)");
        if (fileName.isNull())
            return;

        controller->addAudioClipToNewTrack(fileName);
    });

    // auto pitchItem = new PitchEditorGraphicsItem;
    // QObject::connect(pianoRollView, &TracksGraphicsView::visibleRectChanged, pitchItem,
    //                  &PitchEditorGraphicsItem::setVisibleRect);
    // QObject::connect(pianoRollView, &TracksGraphicsView::scaleChanged, pitchItem, &PitchEditorGraphicsItem::setScale);
    // pianoRollScene->addItem(pitchItem);

    auto splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(controller->tracksView());
    splitter->addWidget(controller->pianoRollView());

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(btnOpenAudioFile);
    mainLayout->addWidget(splitter);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}