#include "Model.h"

#include "paraformer_onnx.h"

namespace FunAsr {
    Model *create_model(const char *path, const int &nThread) {
        return new ModelImp(path, nThread);
    }
}