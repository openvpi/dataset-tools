#include "Model.h"

#include "paraformer_onnx.h"

#include <filesystem>

namespace FunAsr {
    Model *create_model(const std::filesystem::path &path, const int &nThread,
                         ExecutionProvider provider, int deviceId) {
        return new ModelImp(path, nThread, provider, deviceId);
    }
}