#include "librapidasrapi.h"

#include <Model.h>
#include <Audio.h>
#include "commonfunc.h"

#ifdef __cplusplus
//  void __attribute__ ((visibility ("default"))) fun();
extern "C" {
#endif


// APIs for qmasr
_RAPIDASRAPI RPASR_HANDLE RapidAsrInit(const char *szModelDir, int nThreadNum) {
    Model *mm = create_model(szModelDir, nThreadNum);
    return mm;
}

_RAPIDASRAPI RPASR_RESULT RapidAsrRecogBuffer(RPASR_HANDLE handle, const char *szBuf, int nLen,
                                              QM_CALLBACK fnCallback) {
    auto *pRecogObj = (Model *) handle;
    if (!pRecogObj)
        return nullptr;

    Audio audio(1);
    audio.loadwav(szBuf, nLen);
    // audio.split();

    float *buff;
    int len;
    int flag = 0;
    auto *pResult = new RPASR_RECOG_RESULT;
    pResult->snippet_time = audio.get_time_len();
    int nStep = 0;
    const int nTotal = audio.get_queue_size();
    while (audio.fetch(buff, len, flag) > 0) {
        pRecogObj->reset();
        const std::string msg = pRecogObj->forward(buff, len, flag);
        pResult->msg += msg;
        nStep++;
        if (fnCallback)
            fnCallback(nStep, nTotal);
    }
    return pResult;
}

_RAPIDASRAPI const char *RapidAsrGetResult(RPASR_RESULT Result, int nIndex) {
    const RPASR_RECOG_RESULT *pResult = (RPASR_RECOG_RESULT *) Result;
    if (!pResult)
        return nullptr;

    return pResult->msg.c_str();
}

_RAPIDASRAPI void RapidAsrFreeResult(RPASR_RESULT Result) {
    if (Result)
        delete (RPASR_RECOG_RESULT *) Result;
}

_RAPIDASRAPI void RapidAsrUninit(RPASR_HANDLE handle) {
    const Model *pRecogObj = (Model *) handle;

    if (!pRecogObj)
        return;

    delete pRecogObj;
}


#ifdef __cplusplus
}
#endif
