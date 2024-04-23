#include "Common.h"

namespace G2pTest {
    QStringList readData(const QString &filename) {
        QStringList dataLines;
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
#if (QT_VERSION <= QT_VERSION_CHECK(6, 0, 0))
            in.setCodec("UTF-8");
#endif
            while (!in.atEnd()) {
                QString line = in.readLine();
                dataLines.append(line);
            }
            file.close();
        }
        return dataLines;
    }
} // G2pTest