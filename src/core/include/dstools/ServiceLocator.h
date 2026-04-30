#pragma once

#include <dstools/IFileIOProvider.h>

#include <typeindex>
#include <unordered_map>

namespace dstools {

class ServiceLocator {
public:
    /// Register a service instance by interface type.
    template<typename T>
    static void set(T *instance) {
        services()[std::type_index(typeid(T))] = static_cast<void *>(instance);
    }

    /// Retrieve a registered service. Returns nullptr if not registered.
    template<typename T>
    static T *get() {
        auto it = services().find(std::type_index(typeid(T)));
        return it != services().end() ? static_cast<T *>(it->second) : nullptr;
    }

    /// Remove a registered service.
    template<typename T>
    static void reset() {
        services().erase(std::type_index(typeid(T)));
    }

    /// Remove all registered services.
    static void resetAll();

    // Legacy API — kept for backward compatibility
    static IFileIOProvider *fileIO();
    static void setFileIO(IFileIOProvider *provider);
    static void resetFileIO();

private:
    static std::unordered_map<std::type_index, void *> &services();
};

}
