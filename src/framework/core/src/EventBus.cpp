#include <dsfw/EventBus.h>

namespace dstools {

EventBus &EventBus::instance() {
    static EventBus s_instance;
    return s_instance;
}

void EventBus::unsubscribe(int id) {
    std::lock_guard lock(m_mutex);
    for (auto &[key, entries] : m_handlers) {
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                     [id](const HandlerEntry &e) { return e.id == id; }),
                      entries.end());
    }
}

void EventBus::clear() {
    std::lock_guard lock(m_mutex);
    m_handlers.clear();
    m_nextId = 0;
}

} // namespace dstools
