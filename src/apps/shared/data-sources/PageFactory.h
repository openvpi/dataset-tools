#pragma once

#include "IPageDescriptor.h"

#include <QObject>
#include <vector>

namespace dsfw {
    class AppShell;
}

namespace dstools {

    class IEditorDataSource;
    class AppSettingsBackend;

    class PageFactory {
    public:
        static void registerPages(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                                  AppSettingsBackend *settingsBackend,
                                  const std::vector<const IPageDescriptor *> &descriptors);
    };

} // namespace dstools
