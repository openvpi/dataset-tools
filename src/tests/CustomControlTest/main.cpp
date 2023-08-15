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
    progressBar1->setCurrentTaskValue(50);

    auto sliderValue = new QSlider;
    sliderValue->setMaximum(100);
    sliderValue->setMinimum(0);
    sliderValue->setValue(50);
    sliderValue->setOrientation(Qt::Horizontal);
    QObject::connect(sliderValue, &QSlider::valueChanged, progressBar1, &ProgressIndicator::setValue);

    auto sliderSecondaryValue = new QSlider;
    sliderSecondaryValue->setMaximum(100);
    sliderSecondaryValue->setMinimum(0);
    sliderSecondaryValue->setValue(75);
    sliderSecondaryValue->setOrientation(Qt::Horizontal);
    QObject::connect(sliderSecondaryValue, &QSlider::valueChanged, progressBar1, &ProgressIndicator::setSecondaryValue);

    auto sliderCurrentTaskValue = new QSlider;
    sliderCurrentTaskValue->setMaximum(100);
    sliderCurrentTaskValue->setMinimum(0);
    sliderCurrentTaskValue->setValue(50);
    sliderCurrentTaskValue->setOrientation(Qt::Horizontal);
    QObject::connect(sliderCurrentTaskValue, &QSlider::valueChanged, progressBar1, &ProgressIndicator::setCurrentTaskValue);

    auto progressBar2 = new ProgressIndicator;
    progressBar2->setIndeterminate(true);

    auto verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(editLabel1);
    mainLayout->addWidget(editLabel2);
    mainLayout->addWidget(progressBar1);
    mainLayout->addWidget(sliderValue);
    mainLayout->addWidget(sliderSecondaryValue);
    mainLayout->addWidget(sliderCurrentTaskValue);
    mainLayout->addWidget(progressBar2);
    mainLayout->addItem(verticalSpacer);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(500, 200);
    w.show();

    return QApplication::exec();
}