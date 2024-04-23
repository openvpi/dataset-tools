#ifndef DATASET_TOOLS_JYUPTINGTEST_H
#define DATASET_TOOLS_JYUPTINGTEST_H

#include "G2pglobal.h"
#include "CantoneseG2p.h"

namespace G2pTest
{
    class JyuptingTest
    {
    public:
        explicit JyuptingTest();
        ~JyuptingTest();
        bool convertNumTest() const;
        bool removeToneTest() const;
        bool batchTest(const bool& resDisplay = false) const;

    private:
        QScopedPointer<IKg2p::ChineseG2p> g2p_can;
    };
} // G2pTest

#endif // DATASET_TOOLS_JYUPTINGTEST_H
