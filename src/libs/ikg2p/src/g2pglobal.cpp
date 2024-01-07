#include "g2pglobal.h"

#include <QDebug>

namespace IKg2p {

    class G2pGlobal {
    public:
        QString path;
    };

    Q_GLOBAL_STATIC(G2pGlobal, m_global)

    QString dictionaryPath() {
        return m_global->path;
    }

    void setDictionaryPath(const QString &dir) {
        m_global->path = dir;
    }

    bool isLetter(QChar c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    bool isHanzi(QChar c) {
        return (c >= 0x4e00 && c <= 0x9fa5);
    }

    bool isKana(QChar c) {
        return ((c >= 0x3040 && c <= 0x309F) || (c >= 0x30A0 && c <= 0x30FF));
    }

    bool isSpecialKana(QChar c) {
        static QStringView specialKana = QStringLiteral("ャュョゃゅょァィゥェォぁぃぅぇぉ");
        return specialKana.contains(c);
    }

    QList<QStringView> splitString(const QStringView &input) {
        QList<QStringView> res;
        int pos = 0;
        while (pos < input.length()) {
            QChar currentChar = input[pos];
            if (isLetter(currentChar)) {
                int start = pos;
                while (pos < input.length() && isLetter(input[pos])) {
                    pos++;
                }
                res.append(input.mid(start, pos - start));
            } else if (isHanzi(currentChar) || currentChar.isDigit()) {
                res.append(input.mid(pos, 1));
                pos++;
            } else if (isKana(currentChar)) {
                int length = (pos + 1 < input.length() && isSpecialKana(input[pos + 1])) ? 2 : 1;
                res.append(input.mid(pos, length));
                pos += length;
            } else if (!currentChar.isSpace()) {
                res.append(input.mid(pos, 1));
                pos++;
            } else {
                pos++;
            }
        }
        return res;
    }

    bool loadDict(const QString &dict_dir, const QString &fileName, QHash<QString, QString> &resultMap) {
        QString file_path = QDir::cleanPath(dict_dir + "/" + fileName);
        QFile file(file_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << fileName << " error: " << file.errorString();
            return false;
        }

        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            QStringList keyValuePair = line.split(":");
            if (keyValuePair.count() == 2) {
                QString key = keyValuePair[0];
                QString value = keyValuePair[1];
                resultMap[key] = value;
            }
        }
        return true;
    }

    bool loadDict(const QString &dict_dir, const QString &fileName, QHash<QString, QStringList> &resultMap) {
        QString file_path = QDir::cleanPath(dict_dir + "/" + fileName);
        QFile file(file_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << fileName << " error: " << file.errorString();
            return false;
        }

        while (!file.atEnd()) {
            QString line = file.readLine().trimmed();
            QStringList keyValuePair = line.split(":");
            if (keyValuePair.count() == 2) {
                QString key = keyValuePair[0];
                QString value = keyValuePair[1];
                if (value.split(" ").count() > 0) {
                    resultMap[key] = value.split(" ");
                }
            }
        }
        return true;
    }
}
