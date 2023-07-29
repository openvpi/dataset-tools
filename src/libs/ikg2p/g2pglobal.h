#ifndef G2PGLOBAL_H
#define G2PGLOBAL_H

#include <QDir>
#include <QStringList>

namespace IKg2p {

    QString dictionaryPath();

    void setDictionaryPath(const QString &dir);

    QStringList splitString(const QString &input);

    bool loadDict(const QString &dict_dir, const QString &fileName, QHash<QString, QString> &resultMap);
}

#endif // G2PGLOBAL_H