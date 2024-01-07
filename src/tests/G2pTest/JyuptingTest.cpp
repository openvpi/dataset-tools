#include "JyuptingTest.h"
#include "Common.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QTextCodec>


namespace G2pTest {
    JyuptingTest::JyuptingTest() : g2p_can(new IKg2p::Cantonese()) {
    }

    JyuptingTest::~JyuptingTest() {
    }

    bool JyuptingTest::convertNumTest() {
        QString raw1 = "明月@1几32时有##一";
        QString tar1 = "ming jyut jat gei saam ji si jau jat";
        QString res1 = g2p_can->convert(raw1, false, true, IKg2p::errorType::Ignore);
        if (res1 != tar1) {
            qDebug() << "raw1:" << raw1;
            qDebug() << "tar1:" << tar1;
            qDebug() << "res1:" << res1;
            return false;
        }

        QString raw2 = "明月@1几32时有##一";
        QString tar2 = "ming jyut gei si jau jat";
        QString res2 = g2p_can->convert(raw1, false, false, IKg2p::errorType::Ignore);
        if (res2 != tar2) {
            qDebug() << "raw2:" << raw2;
            qDebug() << "tar2:" << tar2;
            qDebug() << "res2:" << res2;
            return false;
        }

        qDebug() << "convertNumTest: success";
        return true;
    }

    bool JyuptingTest::removeToneTest() {
        QString raw1 = "明月@1几32时有##一";
        QString tar1 = "ming4 jyut6 jat1 gei2 saam1 ji6 si4 jau5 jat1";
        QString res1 = g2p_can->convert(raw1, true, true, IKg2p::errorType::Ignore);
        if (res1 != tar1) {
            qDebug() << "raw1:" << raw1;
            qDebug() << "tar1:" << tar1;
            qDebug() << "res1:" << res1;
            return false;
        }

        QString raw2 = "明月@1几32时有##一";
        QString tar2 = "ming4 jyut6 gei2 si4 jau5 jat1";
        QString res2 = g2p_can->convert(raw1, true, false, IKg2p::errorType::Ignore);
        if (res2 != tar2) {
            qDebug() << "raw2:" << raw2;
            qDebug() << "tar2:" << tar2;
            qDebug() << "res2:" << res2;
            return false;
        }

        qDebug() << "removeToneTest: success";
        return true;
    }
    bool JyuptingTest::batchTest(bool resDisplay) {
        int count = 0;
        int error = 0;

        QStringList dataLines;
        dataLines = readData(qApp->applicationDirPath() + "/testData/jyutping_test.txt");

        QElapsedTimer time;
        time.start();
        foreach (const QString &line, dataLines) {
            QStringList keyValuePair = line.trimmed().split('|');

            if (keyValuePair.size() == 2) {
                QString key = keyValuePair[0];

                QString value = keyValuePair[1];
                QString result = g2p_can->convert(key, false, true);

                QStringList words = value.split(" ");
                int wordSize = words.size();
                count += wordSize;

                bool diff = false;
                QStringList resWords = result.split(" ");
                for (int i = 0; i < wordSize; i++) {
                    if (words[i] != resWords[i] && !words[i].split("/").contains(resWords[i])) {
                        diff = true;
                        error++;
                    }
                }

                if (resDisplay && diff) {
                    qDebug() << "text: " << key;
                    qDebug() << "raw: " << value;
                    qDebug() << "res: " << result;
                }
            } else {
                qDebug() << keyValuePair;
            }
        }

        double percentage = ((double) error / (double) count) * 100.0;

        qDebug() << "batchTest: success";
        qDebug() << "batchTest time:" << time.elapsed() << "ms";
        qDebug() << "错误率: " << percentage << "%";
        qDebug() << "错误数: " << error;
        qDebug() << "总字数: " << count;
        return true;
    }

} // G2pTest