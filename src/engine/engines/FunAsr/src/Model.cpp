#include "Model.h"

#include "paraformer_onnx.h"

#include <memory>
#include <filesystem>

namespace FunAsr {
std::unique_ptr<Model> create_model(const std::filesystem::path& path,
                                    const int& nThread,
                                    dsfw::infer::ExecutionProvider provider,
                                    int deviceId) {
    auto model = std::make_unique<ModelImp>(path, nThread, provider, deviceId);
    if (!model->isLoaded()) {
        std::cerr << "Failed to create model: " << model->errorMessage() << std::endl;
        return nullptr;
    }
    return model;
}
}  // namespace FunAsr