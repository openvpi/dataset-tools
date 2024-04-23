#ifndef G2PGLOBAL_H
#define G2PGLOBAL_H

#include <QDir>
#include <QStringList>

namespace IKg2p
{
    QString dictionaryPath();

    void setDictionaryPath(const QString& dir);

    QStringList splitString(const QString& input);

    inline QStringList viewList2strList(const QList<QStringView>& viewList)
    {
        QStringList res;
        res.reserve(viewList.size());
        for (const auto& item : viewList)
        {
            res.push_back(item.toString());
        }
        return res;
    }

    bool loadDict(const QString& dict_dir, const QString& fileName, QHash<QString, QString>& resultMap);

    bool loadDict(const QString& dict_dir, const QString& fileName, QHash<QString, QStringList>& resultMap,
                  const QString& sep1 = ":", const QString& sep2 = " ");

    bool isLetter(QChar c);

    bool isHanzi(QChar c);

    bool isKana(QChar c);

    bool isSpecialKana(QChar c);
}

#endif // G2PGLOBAL_H
