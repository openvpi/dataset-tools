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
    QString testDataPath = qApp->applicationDirPath() + "/testData/op_lab.txt";

    auto g2p_zh = new IKg2p::ZhG2p("mandarin");

    QElapsedTimer time;
    time.start();

    QFile file(testDataPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Can't open the file!";
        return -1;
    }
    QTextStream in(&file);
    QString line;
    while (!in.atEnd()) {
        line = in.readLine();
        QStringList keyValuePair = line.split('|');
        if (keyValuePair.size() == 2) {
            QString key = keyValuePair[0];
            g2p_zh->convert(key, false, false);
        } else {
            qDebug() << keyValuePair;
        }
    }

    qDebug() << "time:" << time.elapsed() << "ms";
    file.close();

    return 0;
}