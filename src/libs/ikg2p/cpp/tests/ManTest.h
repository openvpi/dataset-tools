#ifndef DATASET_TOOLS_MANTEST_H
#define DATASET_TOOLS_MANTEST_H

#include "G2pglobal.h"
#include "MandarinG2p.h"

namespace G2pTest {

    class ManTest {
    public:
        explicit ManTest();
        ~ManTest();
        bool apiTest();
        bool convertNumTest();
        bool removeToneTest();
        bool batchTest(bool resDisplay = false);

    private:
        QScopedPointer<IKg2p::ChineseG2p> g2p_zh;
    };

} // G2pTest

#endif // DATASET_TOOLS_MANTEST_H
