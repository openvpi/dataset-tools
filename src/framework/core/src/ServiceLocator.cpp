#include <dsfw/ServiceLocator.h>

namespace dsfw {

std::unordered_map<std::type_index, std::any> &ServiceLocator::services() {
    static std::unordered_map<std::type_index, std::any> s;
    return s;
}

void ServiceLocator::resetAll() {
    services().clear();
}

} // namespace dsfw

// ─── FormatAdapterRegistry implementation ─────────────────────────────────────

#include <dsfw/FormatAdapterRegistry.h>

namespace dsfw {

FormatAdapterRegistry &FormatAdapterRegistry::instance() {
    static FormatAdapterRegistry s_instance;
    return s_instance;
}

void FormatAdapterRegistry::registerAdapter(std::unique_ptr<IFormatAdapter> adapter) {
    std::lock_guard lock(m_mutex);
    auto id = adapter->formatId();
    auto version = adapter->interfaceVersion();
    if (version != IFormatAdapter::kInterfaceVersion) {
        qWarning("FormatAdapter '%s' reports interface version %d (expected %d); behavior may be undefined",
                 qPrintable(id), version, IFormatAdapter::kInterfaceVersion);
    }
    m_adapters[id] = std::move(adapter);
}

IFormatAdapter *FormatAdapterRegistry::adapter(const QString &formatId) const {
    std::lock_guard lock(m_mutex);
    auto it = m_adapters.find(formatId);
    return it != m_adapters.end() ? it->second.get() : nullptr;
}

QStringList FormatAdapterRegistry::availableFormats() const noexcept {
    std::lock_guard lock(m_mutex);
    QStringList result;
    for (const auto &[id, _] : m_adapters)
        result.append(id);
    return result;
}

} // namespace dsfw
