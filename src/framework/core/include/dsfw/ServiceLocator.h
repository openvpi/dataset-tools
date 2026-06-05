#pragma once

/// @file ServiceLocator.h
/// @brief Type-safe global service registry for dependency injection.

#include <any>
#include <dsfw/FormatAdapterRegistry.h>
#include <typeindex>
#include <unordered_map>

namespace dsfw {

    /// @brief Global service locator for registering and retrieving shared service instances.
    ///
    /// Services are stored by type and retrieved via template parameter.
    /// The locator does not own the service instances; callers manage lifetime.
    ///
    /// @code
/// // Prefer instance() for singletons:
/// auto *mgr = &ModelManager::instance();
///
/// // ServiceLocator for multi-implementation registries:
/// ServiceLocator::set<ITaskProcessor>(myProcessor);
/// auto *proc = ServiceLocator::get<ITaskProcessor>();
/// @endcode
    ///
    /// @note Not thread-safe. Register services before starting worker threads.
    class ServiceLocator {
    public:
        /// @brief Register a service instance by type.
        /// @tparam T Service interface type.
        /// @param instance Pointer to the service (caller retains ownership).
        template <typename T>
        static void set(T *instance) {
            services()[std::type_index(typeid(T))] = instance;
        }

        /// @brief Retrieve a registered service by type.
        /// @tparam T Service interface type.
        /// @return Pointer to the service, or nullptr if not registered or type mismatch.
        template <typename T>
        static T *get() {
            auto it = services().find(std::type_index(typeid(T)));
            if (it == services().end())
                return nullptr;
            auto *p = std::any_cast<T *>(&it->second);
            return p ? *p : nullptr;
        }

        /// @brief Remove a service registration by type.
        /// @tparam T Service interface type.
        template <typename T>
        static void reset() {
            services().erase(std::type_index(typeid(T)));
        }

        /// @brief Remove all service registrations.
        static void resetAll();

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

// Backward compatibility
namespace dstools {
    using dsfw::ServiceLocator;
}
