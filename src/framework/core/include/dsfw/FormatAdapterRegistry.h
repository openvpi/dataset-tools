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

} // namespace dsfw
