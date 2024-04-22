#ifndef MODEL_H
#define MODEL_H

#include <string>

class Model {
public:
    virtual ~Model() = default;
    virtual void reset() = 0;
    virtual std::string forward(float *din, int len, int flag) = 0;
};

Model *create_model(const char *path, const int &nThread = 0);
#endif
