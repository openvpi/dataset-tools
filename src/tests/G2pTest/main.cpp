#include <QApplication>
#include <QDebug>

#include "g2pglobal.h"
#include "zhg2p.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <iostream>
#include <sstream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/dict");

    auto g2p_zh = new IKg2p::ZhG2p("mandarin");

    QStringList raw1 = {"明", "月", "几", "时", "有", "？"};
    QElapsedTimer time;
    time.start();
    for (int i = 0; i < 100000; i++)
        g2p_zh->convert(raw1, false, false);
    qDebug() << "time:" << time.elapsed() << "ms";
    qDebug() << g2p_zh->convert(raw1, false, false);

    return 0;
}