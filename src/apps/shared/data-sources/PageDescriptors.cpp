#include "PageDescriptors.h"

#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"
#include "SettingsPage.h"

#include <AppSettingsBackend.h>
#include <LogPage.h>
#include <dsfw/AppShell.h>

namespace dstools {

using namespace dsfw;

QWidget *MinLabelPageDescriptor::create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                                        AppSettingsBackend *settingsBackend) const {
    auto *page = new MinLabelPage(shell);
    page->setDataSource(dataSource, settingsBackend);
    return page;
}

QWidget *PhonemeLabelerPageDescriptor::create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                                              AppSettingsBackend *settingsBackend) const {
    auto *page = new PhonemeLabelerPage(shell);
    page->setDataSource(dataSource, settingsBackend);
    return page;
}

QWidget *PitchLabelerPageDescriptor::create(dsfw::AppShell *shell, IEditorDataSource *dataSource,
                                            AppSettingsBackend *settingsBackend) const {
    auto *page = new PitchLabelerPage(shell);
    page->setDataSource(dataSource, settingsBackend);
    return page;
}

QWidget *SettingsPageDescriptor::create(dsfw::AppShell *shell, IEditorDataSource * /*dataSource*/,
                                        AppSettingsBackend *settingsBackend) const {
    return new SettingsPage(settingsBackend, shell);
}

QWidget *LogPageDescriptor::create(dsfw::AppShell *shell, IEditorDataSource * /*dataSource*/,
                                   AppSettingsBackend * /*settingsBackend*/) const {
    return new LogPage(shell);
}

} // namespace dstools