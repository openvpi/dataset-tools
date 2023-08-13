#include <QApplication>
#include <QVBoxLayout>
#include "QDebug"

#include "EditLabel.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setFont(QFont("Microsoft Yahei UI", 9));

    MainWindow w;

    auto editLabel1 = new EditLabel;
    auto editLabel2 = new EditLabel("Track 1");
    auto verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(editLabel1);
    mainLayout->addWidget(editLabel2);
    mainLayout->addItem(verticalSpacer);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(500, 200);
    w.show();

    return a.exec();
}