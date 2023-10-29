#include <QApplication>
#include <QDebug>

#include "g2pglobal.h"
#include "zhg2p.h"
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QTextCodec>
#include <QTextStream>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

QStringList readData(const QString &filename) {
    QStringList dataLines;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            dataLines.append(line);
        }
        file.close();
    }
    return dataLines;
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    IKg2p::setDictionaryPath(qApp->applicationDirPath() + "/dict");

    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QStringList dataLines;
    bool mandarin = true;
    bool resDisplay = true;

    IKg2p::ZhG2p *zhG2p;
    if (mandarin) {
        zhG2p = new IKg2p::ZhG2p("mandarin");
        dataLines = readData(R"(D:\projects\dataset-tools\op_lab.txt)");
    } else {
        zhG2p = new IKg2p::ZhG2p("cantonese");
        dataLines = readData(R"(D:\projects\dataset-tools\jyutping_test.txt)");
    }

    int count = 0;
    int error = 0;


    foreach (const QString &line, dataLines) {
        QString trimmedLine = utf8->toUnicode(line.toLocal8Bit()).trimmed();
        QStringList keyValuePair = trimmedLine.split('|');

        if (keyValuePair.size() == 2) {
            QString key = keyValuePair[0];

            QString value = keyValuePair[1];
            QString result = zhG2p->convert(key, false, true);

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

    qDebug() << "错误率: " << percentage << "%";
    qDebug() << "错误数: " << error;
    qDebug() << "总字数: " << count;

    return app.exec();
}