#include "PageFactory.h"

#include <AppSettingsBackend.h>
#include <dsfw/AppShell.h>

namespace dstools {

using namespace dsfw;

void PageFactory::registerPages(
    dsfw::AppShell *shell,
    IEditorDataSource *dataSource,
    AppSettingsBackend *settingsBackend,
    const std::vector<const IPageDescriptor *> &descriptors)
{
    for (const auto *desc : descriptors) {
        auto *page = desc->create(shell, dataSource, settingsBackend);
        shell->addPage(page, desc->id(), {}, desc->title());
    }
}

} // namespace dstools