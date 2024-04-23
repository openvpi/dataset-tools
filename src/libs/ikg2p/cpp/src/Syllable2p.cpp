#include "Syllable2p.h"
#include "Syllable2p_p.h"
#include <QDebug>
#include <utility>

#include "G2pglobal.h"

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#  include <QtCore5Compat>
#endif

namespace IKg2p
{
    Syllable2pPrivate::Syllable2pPrivate(QString phonemeDict, QString sep1, QString sep2) : phonemeDict(
        std::move(phonemeDict)), sep1(std::move(sep1)), sep2(std::move(sep2))
    {
    }

    Syllable2pPrivate::~Syllable2pPrivate() = default;

    void Syllable2pPrivate::init()
    {
        const QFileInfo phonemeDictInfo(phonemeDict);
        loadDict(phonemeDictInfo.absolutePath(), phonemeDictInfo.fileName(), phonemeMap, sep1, sep2);
    }

    Syllable2p::Syllable2p(QString phonemeDict, QString sep1, QString sep2, QObject* parent) : Syllable2p(
        *new Syllable2pPrivate(std::move(phonemeDict), std::move(sep1), std::move(sep2)), parent)
    {
    }

    Syllable2p::~Syllable2p() = default;

    QList<QStringList> Syllable2p::syllableToPhoneme(const QStringList& syllables) const
    {
        Q_D(const Syllable2p);
        QList<QStringList> phonemeList;
        for (const QString& word : syllables)
        {
            phonemeList.append(d->phonemeMap.value(word, QStringList()));
        }
        return phonemeList;
    }


    QStringList Syllable2p::syllableToPhoneme(const QString& syllable) const
    {
        Q_D(const Syllable2p);
        return d->phonemeMap.value(syllable, QStringList());
    }

    Syllable2p::Syllable2p(Syllable2pPrivate& d, QObject* parent) : QObject(parent), d_ptr(&d)
    {
        d.q_ptr = this;

        d.init();
    }
}
