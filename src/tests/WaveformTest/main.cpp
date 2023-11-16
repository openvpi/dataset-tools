//
// Created by fluty on 2023/8/27.
//

#include "AudioClipGraphicsItem.h"


#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "ClipGraphicsItem.h"
#include "TracksGraphicsScene.h"
#include "TracksGraphicsView.h"
#include "WaveformWidget.h"

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    QMainWindow w;

    auto btnOpenAudioFile = new QPushButton;
    btnOpenAudioFile->setText("Open an audio file...");

    auto waveformWidget = new WaveformWidget;
    //    waveformWidget->openFile(QString("D:/Test/Xiaoxiao.wav"));

    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, waveformWidget, [&]() {
        auto fileName =
            QFileDialog::getOpenFileName(btnOpenAudioFile, "Select an Audio File", ".", "Wave Files (*.wav)");
        waveformWidget->openFile(fileName);
    });

    auto clip1 = new AudioClipGraphicsItem;
    clip1->setName("Clip 1");
    clip1->setStart(0);
    clip1->setLength(3840);
    clip1->setClipStart(0);
    clip1->setClipLen(3840);
    clip1->setGain(1.14);
    clip1->setTrackIndex(0);

    auto clip2 = new AudioClipGraphicsItem;
    clip2->setName("Clip 2");
    clip2->setStart(523);
    clip2->setLength(2569);
    clip2->setClipStart(0);
    clip2->setClipLen(2476);
    clip2->setGain(-5.14);
    clip2->setTrackIndex(1);


    auto clip3 = new AudioClipGraphicsItem;
    clip3->setName("Clip 3");
    clip3->setStart(3542);
    clip3->setLength(2500);
    clip3->setClipStart(0);
    clip3->setClipLen(2500);
    clip3->setGain(1.919);
    clip3->setTrackIndex(1);

    auto tracksScene = new TracksGraphicsScene;
    tracksScene->addItem(clip1);
    tracksScene->addItem(clip2);
    tracksScene->addItem(clip3);

    auto tracksView = new TracksGraphicsView;
    tracksView->setMinimumHeight(150);
    tracksView->setScene(tracksScene);
    tracksView->centerOn(0, 0);
    tracksView->setStyleSheet("QGraphicsView { border: none }");

    QObject::connect(tracksView, &TracksGraphicsView::scaleChanged, tracksScene, &TracksGraphicsScene::setScale);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(btnOpenAudioFile);
    mainLayout->addWidget(tracksView);
    mainLayout->addWidget(waveformWidget);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}