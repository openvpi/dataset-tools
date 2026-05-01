#pragma once

/// @file EventBus.h
/// @brief Type-safe cross-module event publish/subscribe system.

#include <any>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace dstools {

/// @brief Type-safe event bus for cross-module communication.
///
/// @code
/// struct FileLoadedEvent { QString path; };
/// auto id = EventBus::instance().subscribe<FileLoadedEvent>([](const auto &e) { ... });
/// EventBus::instance().publish(FileLoadedEvent{"test.wav"});
/// EventBus::instance().unsubscribe(id);
/// @endcode
class EventBus {
public:
    /// @brief Get the singleton instance.
    static EventBus &instance();

    /// @brief Subscribe to events of type Event.
    /// @return Subscription ID for unsubscribe().
    template <typename Event>
    int subscribe(std::function<void(const Event &)> handler) {
        std::lock_guard lock(m_mutex);
        int id = m_nextId++;
        auto key = std::type_index(typeid(Event));
        m_handlers[key].push_back(
            {id, [h = std::move(handler)](const std::any &e) { h(std::any_cast<const Event &>(e)); }});
        return id;
    }

    /// @brief Publish an event to all subscribers.
    template <typename Event>
    void publish(const Event &event) {
        std::vector<HandlerEntry> entries;
        {
            std::lock_guard lock(m_mutex);
            auto key = std::type_index(typeid(Event));
            auto it = m_handlers.find(key);
            if (it != m_handlers.end())
                entries = it->second;
        }
        for (auto &entry : entries)
            entry.callback(event);
    }

    /// @brief Remove a subscription by ID.
    void unsubscribe(int id);
    /// @brief Remove all subscriptions.
    void clear();

private:
    EventBus() = default;
    struct HandlerEntry {
        int id;
        std::function<void(const std::any &)> callback;
    };
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> m_handlers;
    int m_nextId = 0;
    std::mutex m_mutex;
};

} // namespace dstools
