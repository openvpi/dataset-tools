#pragma once

#include <codecvt>
#include <onnxruntime_cxx_api.h>
#include <string>

typedef struct {
    std::string msg;
    float snippet_time;
} RPASR_RECOG_RESULT;

#ifdef _WIN32

inline std::wstring string2wstring(const std::string &str, const std::string &locale) {
    typedef std::codecvt_byname<wchar_t, char, std::mbstate_t> F;
    std::wstring_convert<F> strCnv(new F(locale));
    return strCnv.from_bytes(str);
}

inline std::wstring strToWstr(const std::string &str) {
    if (str.empty())
        return L"";
    return string2wstring(str, "zh-CN");
}

#endif

inline void getInputName(const Ort::Session *session, std::string &inputName, int nIndex = 0) {
    const size_t numInputNodes = session->GetInputCount();
    if (numInputNodes > 0) {
        Ort::AllocatorWithDefaultOptions allocator;
        {
            const auto t = session->GetInputNameAllocated(nIndex, allocator);
            inputName = t.get();
        }
    }
}

inline void getOutputName(const Ort::Session *session, std::string &outputName, int nIndex = 0) {
    const size_t numOutputNodes = session->GetOutputCount();
    if (numOutputNodes > 0) {
        Ort::AllocatorWithDefaultOptions allocator;
        {
            const auto t = session->GetOutputNameAllocated(nIndex, allocator);
            outputName = t.get();
        }
    }
}