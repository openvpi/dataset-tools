#include <dsfw/ServiceLocator.h>
#include <dsfw/FormatAdapterRegistry.h>
#include <dsfw/LocalFileIOProvider.h>

namespace dstools {

std::unordered_map<std::type_index, std::any> &ServiceLocator::services() {
    static std::unordered_map<std::type_index, std::any> s;
    return s;
}

void ServiceLocator::resetAll() {
    services().clear();
    resetFileIO();
}

IFileIOProvider *ServiceLocator::fileIO() {
    return fileIOProvider();
}

void ServiceLocator::setFileIO(IFileIOProvider *provider) {
    setFileIOProvider(provider);
}

void ServiceLocator::resetFileIO() {
    resetFileIOProvider();
}

FormatAdapterRegistry *ServiceLocator::formatAdapterRegistry() {
    return get<FormatAdapterRegistry>();
}

void ServiceLocator::setFormatAdapterRegistry(FormatAdapterRegistry *registry) {
    set<FormatAdapterRegistry>(registry);
}

void ServiceLocator::resetFormatAdapterRegistry() {
    reset<FormatAdapterRegistry>();
}

} // namespace dstools

// ─── FormatAdapterRegistry implementation ─────────────────────────────────────

namespace dstools {

FormatAdapterRegistry &FormatAdapterRegistry::instance() {
    auto *registry = ServiceLocator::formatAdapterRegistry();
    if (!registry) {
        static FormatAdapterRegistry defaultRegistry;
        ServiceLocator::setFormatAdapterRegistry(&defaultRegistry);
        return defaultRegistry;
    }
    return *registry;
}

void FormatAdapterRegistry::registerAdapter(std::unique_ptr<IFormatAdapter> adapter) {
    std::lock_guard lock(m_mutex);
    auto id = adapter->formatId();
    m_adapters[id] = std::move(adapter);
}

IFormatAdapter *FormatAdapterRegistry::adapter(const QString &formatId) const {
    std::lock_guard lock(m_mutex);
    auto it = m_adapters.find(formatId);
    return it != m_adapters.end() ? it->second.get() : nullptr;
}

QStringList FormatAdapterRegistry::availableFormats() const {
    std::lock_guard lock(m_mutex);
    QStringList result;
    for (const auto &[id, _] : m_adapters)
        result.append(id);
    return result;
}

} // namespace dstools
