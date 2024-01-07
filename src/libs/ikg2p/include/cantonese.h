#ifndef DATASET_TOOLS_CANTONESE_H
#define DATASET_TOOLS_CANTONESE_H

#include "zhg2p.h"
namespace IKg2p {
    class Cantonese : public ZhG2p {
    public:
        explicit Cantonese(QObject *parent = nullptr) : ZhG2p("cantonese", parent){};
        ~Cantonese() override = default;
    };
}
#endif // DATASET_TOOLS_CANTONESE_H
