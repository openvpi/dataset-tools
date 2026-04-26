#pragma once

#include "SettingsPage.h"

#include <QObject>

namespace dsfw {
    class AppShell;
}

namespace dstools {

    class IEditorDataSource;
    class AppSettingsBackend;

    class PageFactory {
    public:
        static void registerSharedEditorPages(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                                              AppSettingsBackend *settingsBackend);

        static SettingsPage *registerUtilityPages(dsfw::AppShell *shell, AppSettingsBackend *settingsBackend);
    };

} // namespace dstools
