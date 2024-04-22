#include "Model.h"

#include "paraformer_onnx.h"

Model *create_model(const char *path, const int &nThread) {
    return new paraformer::ModelImp(path, nThread);
}
