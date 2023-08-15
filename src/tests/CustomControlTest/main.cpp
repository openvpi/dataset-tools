#include "QDebug"
#include <QApplication>
#include <QSlider>
#include <QVBoxLayout>

#include "EditLabel.h"
#include "MainWindow.h"
#include "ProgressIndicator.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QApplication::setFont(QFont("Microsoft Yahei UI", 9));

    MainWindow w;

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

    auto sliderValue = new QSlider;
    sliderValue->setMaximum(100);
    sliderValue->setMinimum(0);
    sliderValue->setValue(50);
    sliderValue->setOrientation(Qt::Horizontal);
    QObject::connect(sliderValue, &QSlider::valueChanged, &w, [=](int value) {
        progressBar1->setValue(value);
        progressBar3->setValue(value);
        progressBar5->setValue(value);
    });

    auto sliderSecondaryValue = new QSlider;
    sliderSecondaryValue->setMaximum(100);
    sliderSecondaryValue->setMinimum(0);
    sliderSecondaryValue->setValue(75);
    sliderSecondaryValue->setOrientation(Qt::Horizontal);
    QObject::connect(sliderSecondaryValue, &QSlider::valueChanged, &w, [=](int value) {
        progressBar1->setSecondaryValue(value);
        progressBar3->setSecondaryValue(value);
        progressBar5->setSecondaryValue(value);
    });

    auto sliderCurrentTaskValue = new QSlider;
    sliderCurrentTaskValue->setMaximum(100);
    sliderCurrentTaskValue->setMinimum(0);
    sliderCurrentTaskValue->setValue(25);
    sliderCurrentTaskValue->setOrientation(Qt::Horizontal);
    QObject::connect(sliderCurrentTaskValue, &QSlider::valueChanged, &w, [=](int value) {
        progressBar1->setCurrentTaskValue(value);
        progressBar3->setCurrentTaskValue(value);
        progressBar5->setCurrentTaskValue(value);
    });

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
    mainLayout->addWidget(sliderValue);
    mainLayout->addWidget(sliderSecondaryValue);
    mainLayout->addWidget(sliderCurrentTaskValue);
//    mainLayout->addItem(verticalSpacer);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(300, 500);
    w.show();

    return QApplication::exec();
}