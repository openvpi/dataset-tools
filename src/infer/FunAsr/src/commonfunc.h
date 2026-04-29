#pragma once

#include <onnxruntime_cxx_api.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace FunAsr {
    typedef struct {
        std::string msg;
        float snippet_time;
    } RPASR_RECOG_RESULT;

#ifdef _WIN32

    inline std::wstring strToWstr(const std::string &str) {
        if (str.empty())
            return L"";
        int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
        if (len <= 0)
            return L"";
        std::wstring result(len - 1, L'\0');
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &result[0], len);
        return result;
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
}