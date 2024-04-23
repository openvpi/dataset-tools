#ifndef SYLLABLE2PPRIVATE_H
#define SYLLABLE2PPRIVATE_H

#include <QHash>

#include "Syllable2p.h"

namespace IKg2p {

    class Syllable2pPrivate {
        Q_DECLARE_PUBLIC(Syllable2p)

    public:
        explicit Syllable2pPrivate(QString phonemeDict, QString sep1 = "\t", QString sep2 = " ");

        ~Syllable2pPrivate();

        void init();

        Syllable2p *q_ptr;
    private:
        QString phonemeDict;
        QString sep1;
        QString sep2;
        QHash<QString, QStringList> phonemeMap;
    };
}
#endif // SYLLABLE2PPRIVATE_H
