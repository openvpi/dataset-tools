#ifndef DATASET_TOOLS_MANDARIN_H
#define DATASET_TOOLS_MANDARIN_H

#include "zhg2p.h"
namespace IKg2p {
    class Mandarin : public ZhG2p {
    public:
        explicit Mandarin(QObject *parent = nullptr) : ZhG2p("mandarin", parent){};
        ~Mandarin() override = default;
    };
}

#endif // DATASET_TOOLS_MANDARIN_H
