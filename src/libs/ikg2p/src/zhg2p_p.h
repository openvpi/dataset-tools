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

        ZhG2p *q_ptr{};

        QHash<QString, QString> phrases_map;
        QHash<QString, QStringList> phrases_dict;
        QHash<QString, QStringList> word_dict;
        QHash<QString, QString> trans_dict;

        // Key as QStringView
        QHash<QStringView, QStringView> phrases_map2;
        QHash<QStringView, QList<QStringView>> phrases_dict2;
        QHash<QStringView, QList<QStringView>> word_dict2;
        QHash<QStringView, QStringView> trans_dict2;

        QString m_language;

        inline bool isPolyphonic(const QStringView &text) const {
            return phrases_map2.contains(text);
        }

        inline QStringView tradToSim(const QStringView &text) const {
            return trans_dict2.value(text, text);
        }

        inline QStringView getDefaultPinyin(const QStringView &text) const {
            return word_dict2.value(text, {}).at(0);
        }

        void zhPosition(const QList<QStringView> &input, QList<QStringView> &res, QList<int> &positions,
                        bool convertNum) const;
    };

}

#endif // ZHG2PPRIVATE_H