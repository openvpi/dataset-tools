#ifndef ZHG2PPRIVATE_H
#define ZHG2PPRIVATE_H

#include <QHash>

#include "zhg2p.h"

namespace IKg2p {

    class ZhG2pPrivate {
        Q_DECLARE_PUBLIC(ZhG2p)
    public:
        ZhG2pPrivate(QString language);
        virtual ~ZhG2pPrivate();

        void init();

        ZhG2p *q_ptr;

        QHash<QString, QString> phrases_map;
        QHash<QString, QString> phrases_dict;
        QHash<QString, QString> word_dict;
        QHash<QString, QString> trans_dict;

        QString m_language;

        bool isPolyphonic(const QString &text) const;
        QString tradToSim(const QString &text) const;
        QString getDefaultPinyin(const QString &text) const;
        void zhPosition(const QStringList &input, QStringList &res, QList<int> &positions) const;

    };

}

#endif // ZHG2PPRIVATE_H