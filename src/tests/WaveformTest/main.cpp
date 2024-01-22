//
// Created by fluty on 2023/8/27.
//

#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "TracksController.h"
#include "TracksGraphicsView.h"
#include "AudioClipGraphicsItem.h"

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

    auto btnOpenAudioFile = new QPushButton;
    btnOpenAudioFile->setText("Add...");

    auto tracksController = new TracksController;

    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, tracksController, [&]() {
        auto fileName =
            QFileDialog::getOpenFileName(btnOpenAudioFile, "Select an Audio File", ".",
                "All Audio File (*.wav *.flac *.mp3);;Wave File (*.wav);;Flac File (*.flac);;MP3 File (*.mp3)");
        if (fileName.isNull())
            return;

        tracksController->addAudioClipToNewTrack(fileName);
    });

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(btnOpenAudioFile);
    mainLayout->addWidget(tracksController->tracksView());

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}