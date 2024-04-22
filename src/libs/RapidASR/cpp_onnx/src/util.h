#ifndef UTIL_H
#define UTIL_H

#include <string>
#include "Tensor.h"

extern float *loadparams(const char *filename);

extern void SaveDataFile(const char *filename, void *data, uint32_t len);
extern void relu(Tensor<float> *din);
extern void swish(Tensor<float> *din);
extern void sigmoid(Tensor<float> *din);
extern void doubleswish(Tensor<float> *din);

extern void softmax(float *din, int mask, int len);

extern void log_softmax(float *din, int len);
extern int val_align(int val, int align);
extern void disp_params(float *din, int size);

extern void basic_norm(Tensor<float> *&din, float norm);

extern void findmax(float *din, int len, float &max_val, int &max_idx);

extern void glu(Tensor<float> *din, Tensor<float> *dout);

std::string pathAppend(const std::string &p1, const std::string &p2);

#endif
