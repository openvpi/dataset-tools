#ifndef DATASET_TOOLS_JYUPTINGTEST_H
#define DATASET_TOOLS_JYUPTINGTEST_H

#include "g2pglobal.h"
#include "cantonese.h"

namespace G2pTest {

    class JyuptingTest {
    public:
        explicit JyuptingTest();
        ~JyuptingTest();
        bool convertNumTest();
        bool removeToneTest();
        bool batchTest(bool resDisplay = false);

    private:
        QScopedPointer<IKg2p::ZhG2p> g2p_can;
    };

} // G2pTest

#endif // DATASET_TOOLS_JYUPTINGTEST_H
