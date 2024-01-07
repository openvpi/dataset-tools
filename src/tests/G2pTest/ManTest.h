#ifndef DATASET_TOOLS_MANTEST_H
#define DATASET_TOOLS_MANTEST_H

#include "g2pglobal.h"
#include "mandarin.h"

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
        QScopedPointer<IKg2p::ZhG2p> g2p_zh;
    };

} // G2pTest

#endif // DATASET_TOOLS_MANTEST_H
