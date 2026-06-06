#pragma once

#include <QString>
#include <QStringList>
#include <dsfw/IFormatAdapter.h>
#include <map>
#include <memory>
#include <mutex>

namespace dsfw {

    class FormatAdapterRegistry {
    public:
        static FormatAdapterRegistry &instance();

        void registerAdapter(std::unique_ptr<IFormatAdapter> adapter);
        IFormatAdapter *adapter(const QString &formatId) const;
        QStringList availableFormats() const noexcept;

    private:
        FormatAdapterRegistry() = default;
        mutable std::mutex m_mutex;
        std::map<QString, std::unique_ptr<IFormatAdapter>> m_adapters;
    };

    /// RAII helper that registers a format adapter on construction.
    struct FormatAdapterRegistrar {
        explicit FormatAdapterRegistrar(std::unique_ptr<IFormatAdapter> adapter) {
            FormatAdapterRegistry::instance().registerAdapter(std::move(adapter));
        }
    };

} // namespace dsfw

/// @brief Statically register a format adapter so it is discovered automatically.
/// Place this macro in the adapter's .cpp file.
#define REGISTER_FORMAT_ADAPTER(AdapterClass) \
    static dsfw::FormatAdapterRegistrar _registrar_##AdapterClass(std::make_unique<AdapterClass>())
