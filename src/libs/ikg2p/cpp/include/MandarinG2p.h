#ifndef DATASET_TOOLS_MANDARIN_H
#define DATASET_TOOLS_MANDARIN_H

#include "ChineseG2p.h"

namespace IKg2p
{
    class MandarinG2p final : public ChineseG2p
    {
    public:
        explicit MandarinG2p(QObject* parent = nullptr) : ChineseG2p("mandarin", parent)
        {
        };
        ~MandarinG2p() override = default;
    };
}

#endif // DATASET_TOOLS_MANDARIN_H
