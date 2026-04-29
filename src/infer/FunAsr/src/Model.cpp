#include "Model.h"

#include "paraformer_onnx.h"

#include <filesystem>

namespace FunAsr {
    Model *create_model(const std::filesystem::path &path, const int &nThread,
                         ExecutionProvider provider, int deviceId) {
        auto *model = new ModelImp(path, nThread, provider, deviceId);
        if (!model->isLoaded()) {
            std::cerr << "Failed to create model: " << model->errorMessage() << std::endl;
            delete model;
            return nullptr;
        }
        return model;
    }
}