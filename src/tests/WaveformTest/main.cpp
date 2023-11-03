//
// Created by fluty on 2023/8/27.
//

#include "WaveformWidget.h"
#include <QApplication>
#include <QFileDialog>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

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

    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, waveformWidget, [&](){
        auto fileName = QFileDialog::getOpenFileName(btnOpenAudioFile, "Select an Audio File", ".", "Wave Files (*.wav)");
        waveformWidget->openFile(fileName);
    });

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(btnOpenAudioFile);
    mainLayout->addWidget(waveformWidget);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 400);
    w.show();

    return QApplication::exec();
}