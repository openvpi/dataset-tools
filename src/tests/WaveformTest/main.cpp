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

    auto tracksScene = new TracksGraphicsScene;

    auto tracksView = new TracksGraphicsView;
    tracksView->setMinimumHeight(150);
    tracksView->setScene(tracksScene);
    tracksView->centerOn(0, 0);
    tracksView->setStyleSheet("QGraphicsView { border: none }");

    int trackCount = 0;

    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, tracksView, [&]() {
        auto fileName =
            QFileDialog::getOpenFileName(btnOpenAudioFile, "Select an Audio File", ".", "Wave Files (*.wav)");
        auto clip = new AudioClipGraphicsItem;
        clip->setStart(0);
        clip->setGain(1.14);
        clip->setTrackIndex(trackCount);
        clip->openFile(fileName);
        clip->setVisibleRect(tracksView->visibleRect());
        clip->setScaleX(tracksView->scaleX());
        clip->setScaleY(tracksView->scaleY());
        tracksScene->addItem(clip);
        QObject::connect(tracksView, &TracksGraphicsView::visibleRectChanged, clip, &ClipGraphicsItem::setVisibleRect);
        trackCount++;
    });

    QObject::connect(tracksView, &TracksGraphicsView::scaleChanged, tracksScene, &TracksGraphicsScene::setScale);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(btnOpenAudioFile);
    mainLayout->addWidget(tracksView);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}