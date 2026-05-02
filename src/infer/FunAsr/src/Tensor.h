#ifndef TENSOR_H
#define TENSOR_H

#include <cstring>
#include <iostream>

#include "alignedmem.h"
namespace FunAsr {
    template <typename T>
    class Tensor {
    private:
        void alloc_buff();
        void free_buff() const;
        int mem_size{};

    public:
        T *buff;
        int size[4]{};
        int buff_size{};
        explicit Tensor(Tensor *in);
        explicit Tensor(const int &a);
        Tensor(const int &a, const int &b);
        Tensor(const int &a, const int &b, const int &c);
        Tensor(const int &a, const int &b, const int &c, const int &d);
        ~Tensor();
    };

    template <typename T>
    Tensor<T>::Tensor(const int &a) : size{1, 1, 1, a} {
        alloc_buff();
    }

    template <typename T>
    Tensor<T>::Tensor(const int &a, const int &b) : size{1, 1, a, b} {
        alloc_buff();
    }

    template <typename T>
    Tensor<T>::Tensor(const int &a, const int &b, const int &c) : size{1, a, b, c} {
        alloc_buff();
    }

    template <typename T>
    Tensor<T>::Tensor(const int &a, const int &b, const int &c, const int &d) : size{a, b, c, d} {
        alloc_buff();
    }

    template <typename T>
    Tensor<T>::Tensor(Tensor *in) {
        memcpy(size, in->size, 4 * sizeof(int));
        alloc_buff();
        memcpy(buff, in->buff, in->buff_size * sizeof(T));
    }

    template <typename T>
    Tensor<T>::~Tensor() {
        free_buff();
    }

    template <typename T>
    void Tensor<T>::alloc_buff() {
        buff_size = size[0] * size[1] * size[2] * size[3];
        mem_size = buff_size;
        buff = static_cast<T *>(aligned_malloc(32, buff_size * sizeof(T)));
    }

    template <typename T>
    void Tensor<T>::free_buff() const {
        aligned_free(buff);
    }
}
#endif
