#pragma once

/// @file ServiceLocator.h
/// @brief Type-safe global service registry for dependency injection.

#include <dsfw/IFileIOProvider.h>
#include <dsfw/FormatAdapterRegistry.h>

#include <any>
#include <typeindex>
#include <unordered_map>

namespace dstools {

/// @brief Global service locator for registering and retrieving shared service instances.
///
/// Services are stored by type and retrieved via template parameter.
/// The locator does not own the service instances; callers manage lifetime.
///
/// @code
/// ServiceLocator::set<IFileIOProvider>(myProvider);
/// auto *io = ServiceLocator::get<IFileIOProvider>();
/// @endcode
///
/// @note Not thread-safe. Register services before starting worker threads.
class ServiceLocator {
public:
    /// @brief Register a service instance by type.
    /// @tparam T Service interface type.
    /// @param instance Pointer to the service (caller retains ownership).
    template<typename T>
    static void set(T *instance) {
        services()[std::type_index(typeid(T))] = instance;
    }

    /// @brief Retrieve a registered service by type.
    /// @tparam T Service interface type.
    /// @return Pointer to the service, or nullptr if not registered or type mismatch.
    template<typename T>
    static T *get() {
        auto it = services().find(std::type_index(typeid(T)));
        if (it == services().end())
            return nullptr;
        auto *p = std::any_cast<T *>(&it->second);
        return p ? *p : nullptr;
    }

    /// @brief Remove a service registration by type.
    /// @tparam T Service interface type.
    template<typename T>
    static void reset() {
        services().erase(std::type_index(typeid(T)));
    }

    /// @brief Remove all service registrations.
    static void resetAll();

    /// @brief Convenience: get the registered IFileIOProvider.
    /// @return Pointer to the file I/O provider, or nullptr.
    static IFileIOProvider *fileIO();
    /// @brief Convenience: register an IFileIOProvider.
    /// @param provider Pointer to the provider (caller retains ownership).
    static void setFileIO(IFileIOProvider *provider);
    /// @brief Convenience: remove the IFileIOProvider registration.
    static void resetFileIO();

    /// @brief Convenience: get the registered FormatAdapterRegistry.
    /// @return Pointer to the registry, or nullptr.
    static FormatAdapterRegistry *formatAdapterRegistry();
    /// @brief Convenience: register a FormatAdapterRegistry.
    /// @param registry Pointer to the registry (caller retains ownership).
    static void setFormatAdapterRegistry(FormatAdapterRegistry *registry);
    /// @brief Convenience: remove the FormatAdapterRegistry registration.
    static void resetFormatAdapterRegistry();

private:
    static std::unordered_map<std::type_index, std::any> &services();
};

}
