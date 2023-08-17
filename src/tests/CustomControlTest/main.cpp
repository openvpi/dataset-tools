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

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QApplication::setFont(QFont("Microsoft Yahei UI", 9));

    MainWindow w;

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
    mainLayout->addWidget(shadowButton);
    mainLayout->addLayout(switchButtonLayout);
//    mainLayout->addItem(verticalSpacer);

    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(300, 700);
    w.show();

    return QApplication::exec();
}