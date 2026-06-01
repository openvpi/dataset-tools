#pragma once

#include <QString>
#include <QWidget>

namespace dsfw {
    class AppShell;
}

namespace dstools {

    class IEditorDataSource;
    class AppSettingsBackend;

    class IPageDescriptor {
    public:
        static constexpr int kInterfaceVersion = 1;

        virtual ~IPageDescriptor() = default;

        virtual QWidget *create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                                AppSettingsBackend *settingsBackend) const = 0;

        virtual QString id() const = 0;
        virtual QString title() const = 0;
    };

} // namespace dstools