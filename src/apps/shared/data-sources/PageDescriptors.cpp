#include "PageDescriptors.h"

#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"
#include "SettingsPage.h"

#include <AppSettingsBackend.h>
#include <LogPage.h>
#include <dsfw/AppShell.h>

namespace dstools {


QWidget *MinLabelPageDescriptor::create(dsfw::AppShell *shell, dsfw::IEditorDataSource *dataSource,
                                        AppSettingsBackend *settingsBackend) const {
    auto *page = new dsfw::MinLabelPage(shell);
    page->setDataSource(dataSource, settingsBackend);
    return page;
}

QWidget *PhonemeLabelerPageDescriptor::create(dsfw::AppShell *shell, dsfw::IEditorDataSource *dataSource,
                                              AppSettingsBackend *settingsBackend) const {
    auto *page = new dsfw::PhonemeLabelerPage(shell);
    page->setDataSource(dataSource, settingsBackend);
    return page;
}

QWidget *PitchLabelerPageDescriptor::create(dsfw::AppShell *shell, dsfw::IEditorDataSource *dataSource,
                                            AppSettingsBackend *settingsBackend) const {
    auto *page = new dsfw::PitchLabelerPage(shell);
    page->setDataSource(dataSource, settingsBackend);
    return page;
}

QWidget *SettingsPageDescriptor::create(dsfw::AppShell *shell, dsfw::IEditorDataSource * /*dataSource*/,
                                        AppSettingsBackend *settingsBackend) const {
    return new SettingsPage(settingsBackend, shell);
}

QWidget *LogPageDescriptor::create(dsfw::AppShell *shell, dsfw::IEditorDataSource * /*dataSource*/,
                                   AppSettingsBackend * /*settingsBackend*/) const {
    return new LogPage(shell);
}

} // namespace dstools
