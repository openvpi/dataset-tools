#ifndef ChineseG2pPRIVATE_H
#define ChineseG2pPRIVATE_H

#include <QHash>

#include "ChineseG2p_p.h"

namespace IKg2p
{
    class ChineseG2pPrivate final
    {
        Q_DECLARE_PUBLIC(ChineseG2p)

    public:
        explicit ChineseG2pPrivate(QString language);
        virtual ~ChineseG2pPrivate();

        void init();

        ChineseG2p* q_ptr{};

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

        [[nodiscard]] inline bool isPolyphonic(const QStringView& text) const
        {
            return phrases_map2.contains(text);
        }

        [[nodiscard]] inline QStringView tradToSim(const QStringView& text) const
        {
            return trans_dict2.value(text, text);
        }

        [[nodiscard]] inline QStringView getDefaultPinyin(const QStringView& text) const
        {
            return word_dict2.value(text, {}).at(0);
        }

        void zhPosition(const QList<QStringView>& input, QList<QStringView>& res, QList<int>& positions,
                        const bool& convertNum) const;
    };
}

#endif // ChineseG2pPRIVATE_H
