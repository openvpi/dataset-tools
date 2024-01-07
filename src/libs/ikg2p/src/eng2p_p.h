#ifndef ENG2PPRIVATE_H
#define ENG2PPRIVATE_H
#include <QHash>

#include "eng2p.h"

namespace IKg2p {

    class EnG2pPrivate {
        Q_DECLARE_PUBLIC(EnG2p)
    public:
        EnG2pPrivate();
        virtual ~EnG2pPrivate();

        void init();

        EnG2p *q_ptr;

        QHash<QString, QString> arpabetMap;
    };
}
#endif // ENG2PPRIVATE_H
