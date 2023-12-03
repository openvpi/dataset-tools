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

        std::unordered_map<std::string, std::string> phrases_map;
        std::unordered_map<std::string, std::string> phrases_dict;
        std::unordered_map<std::string, std::string> word_dict;
        std::unordered_map<std::string, std::string> trans_dict;

        QString m_language;

        bool isPolyphonic(const std::string &text) const;
        std::string tradToSim(const std::string &text) const;
        std::string getDefaultPinyin(const std::string &text) const;
        void zhPosition(const std::vector<std::string> &input, std::vector<std::string> &res,
                        std::vector<int> &positions, bool covertNum) const;
    };

}

#endif // ZHG2PPRIVATE_H