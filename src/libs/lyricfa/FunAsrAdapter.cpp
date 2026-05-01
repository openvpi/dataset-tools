#include "FunAsrAdapter.h"

#include <Model.h>

namespace LyricFA {

FunAsrAdapter::~FunAsrAdapter() = default;

bool FunAsrAdapter::isOpen() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model != nullptr;
}

const char *FunAsrAdapter::engineName() const {
    return "FunASR";
}

bool FunAsrAdapter::load(const std::filesystem::path &modelPath,
                          dstools::infer::ExecutionProvider provider, int deviceId,
                          std::string &errorMsg) {
    unload();

    auto *raw = FunAsr::create_model(modelPath, 4, provider, deviceId);
    if (!raw) {
        errorMsg = "Failed to load FunASR model from: " + modelPath.string();
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_model.reset(raw);
    return true;
}

void FunAsrAdapter::unload() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_model.reset();
}

FunAsr::Model *FunAsrAdapter::model() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_model.get();
}

} // namespace LyricFA
