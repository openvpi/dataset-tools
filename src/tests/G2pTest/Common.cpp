#include "Common.h"

namespace G2pTest {
    QStringList readData(const QString &filename) {
        QStringList dataLines;
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            in.setCodec("UTF-8");
            while (!in.atEnd()) {
                QString line = in.readLine();
                dataLines.append(line);
            }
            file.close();
        }
        return dataLines;
    }
} // G2pTest