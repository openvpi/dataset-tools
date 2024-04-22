#include "Model.h"

#include "paraformer_onnx.h"

Model *create_model(const char *path, int nThread) {
    return new paraformer::ModelImp(path, nThread);
}
