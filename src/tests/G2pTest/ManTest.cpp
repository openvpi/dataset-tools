#include "ManTest.h"
#include "Common.h"

#include <QDebug>
#include <QTextCodec>

namespace G2pTest {
    ManTest::ManTest() : g2p_zh(new IKg2p::ZhG2p("mandarin")) {
    }

    ManTest::~ManTest() {
    }

    bool ManTest::convertNumTest() {
        QString raw1 = "明月@1几32时有##一";
        QString tar1 = "ming yue yi ji san er shi you yi";
        QString res1 = g2p_zh->convert(raw1, false, true);
        if (res1 != tar1) {
            qDebug() << "raw1:" << raw1;
            qDebug() << "tar1:" << tar1;
            qDebug() << "res1:" << res1;
            return false;
        }

        QString raw2 = "明月@1几32时有##一";
        QString tar2 = "ming yue 1 ji 3 2 shi you yi";
        QString res2 = g2p_zh->convert(raw1, false, false);
        if (res2 != tar2) {
            qDebug() << "raw2:" << raw2;
            qDebug() << "tar2:" << tar2;
            qDebug() << "res2:" << res2;
            return false;
        }

        qDebug() << "\nconvertNumTest: success";
        return true;
    }

    bool ManTest::removeToneTest() {
        QString raw1 = "明月@1几32时有##一";
        QString tar1 = "ming2 yue4 yi1 ji3 san1 er4 shi2 you3 yi1";
        QString res1 = g2p_zh->convert(raw1, true, true);
        if (res1 != tar1) {
            qDebug() << "raw1:" << raw1;
            qDebug() << "tar1:" << tar1;
            qDebug() << "res1:" << res1;
            return false;
        }

        QString raw2 = "明月@1几32时有##一";
        QString tar2 = "ming2 yue4 1 ji3 3 2 shi2 you3 yi1";
        QString res2 = g2p_zh->convert(raw1, true, false);
        if (res2 != tar2) {
            qDebug() << "raw2:" << raw2;
            qDebug() << "tar2:" << tar2;
            qDebug() << "res2:" << res2;
            return false;
        }

        qDebug() << "\nremoveToneTest: success";
        return true;
    }
    bool ManTest::batchTest(bool resDisplay) {
        int count = 0;
        int error = 0;

        QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
        QStringList dataLines;
        dataLines = readData(R"(D:\projects\dataset-tools\op_lab.txt)");
        foreach (const QString &line, dataLines) {

            QString trimmedLine = utf8->toUnicode(line.toLocal8Bit()).trimmed();
            QStringList keyValuePair = trimmedLine.split('|');

            if (keyValuePair.size() == 2) {
                QString key = keyValuePair[0];

                QString value = keyValuePair[1];
                QString result = g2p_zh->convert(key, false, true);

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

        qDebug() << "\n--------------------";
        qDebug() << "batchTest: success";
        qDebug() << "错误率: " << percentage << "%";
        qDebug() << "错误数: " << error;
        qDebug() << "总字数: " << count;
        qDebug() << "--------------------";
        return true;
    }

} // G2pTest