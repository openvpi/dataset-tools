#pragma once

#include <dsfw/IFileIOProvider.h>

#include <typeindex>
#include <unordered_map>

namespace dstools {

class ServiceLocator {
public:
    template<typename T>
    static void set(T *instance) {
        services()[std::type_index(typeid(T))] = static_cast<void *>(instance);
    }

    template<typename T>
    static T *get() {
        auto it = services().find(std::type_index(typeid(T)));
        return it != services().end() ? static_cast<T *>(it->second) : nullptr;
    }

    template<typename T>
    static void reset() {
        services().erase(std::type_index(typeid(T)));
    }

    static void resetAll();

    static IFileIOProvider *fileIO();
    static void setFileIO(IFileIOProvider *provider);
    static void resetFileIO();

private:
    static std::unordered_map<std::type_index, void *> &services();
};

}
