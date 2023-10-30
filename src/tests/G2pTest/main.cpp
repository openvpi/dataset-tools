#include <QApplication>
#include <QDebug>

#include "g2pglobal.h"
#include "zhg2p.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QTextCodec>
#include <QTextStream>
#include <fstream>
#include <sstream>

#include "JyuptingTest.h"
#include "ManTest.h"

using namespace G2pTest;

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/dict");

    qDebug() << "**************************";
    qDebug() << "mandarinTest: ";
    auto mandarinTest = ManTest();
    mandarinTest.convertNumTest();
    mandarinTest.removeToneTest();
    //    mandarinTest.batchTest();
    qDebug() << "\n";

    qDebug() << "**************************";

    qDebug() << "jyutpingTest: ";
    auto jyutpingTest = JyuptingTest();
    jyutpingTest.convertNumTest();
    jyutpingTest.removeToneTest();
    //    jyutpingTest.batchTest();
    qDebug() << "\n";

    return 0;
}
