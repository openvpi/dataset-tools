#include "librapidasrapi.h"

#include "commonfunc.h"
#include <Audio.h>
#include <Model.h>

#ifdef __cplusplus
//  void __attribute__ ((visibility ("default"))) fun();
extern "C" {
#endif


// APIs for qmasr
_RAPIDASRAPI RPASR_HANDLE RapidAsrInit(const char *szModelDir, const int &nThread) {
    Model *mm = create_model(szModelDir, nThread);
    return mm;
}

_RAPIDASRAPI RPASR_RESULT RapidAsrRecogBuffer(const RPASR_HANDLE &handle, const char *szBuf, const int &nLen,
                                              const QM_CALLBACK &fnCallback) {
    auto *pRecogObj = static_cast<Model *>(handle);
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

_RAPIDASRAPI const char *RapidAsrGetResult(const RPASR_RESULT &Result) {
    const RPASR_RECOG_RESULT *pResult = static_cast<RPASR_RECOG_RESULT *>(Result);
    if (!pResult)
        return nullptr;

    return pResult->msg.c_str();
}

_RAPIDASRAPI void RapidAsrFreeResult(const RPASR_RESULT &Result) {
    if (Result)
        delete static_cast<RPASR_RECOG_RESULT *>(Result);
}

_RAPIDASRAPI void RapidAsrUninit(const RPASR_HANDLE &handle) {
    const Model *pRecogObj = static_cast<Model *>(handle);

    if (!pRecogObj)
        return;

    delete pRecogObj;
}

#ifdef __cplusplus
}
#endif
