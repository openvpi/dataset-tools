#include "PageFactory.h"

#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"

#include <AppSettingsBackend.h>
#include <SettingsPage.h>
#include <LogPage.h>

#include <dsfw/AppShell.h>

namespace dstools {

void PageFactory::registerSharedEditorPages(
    dsfw::AppShell *shell,
    IEditorDataSource *dataSource,
    AppSettingsBackend *settingsBackend)
{
    auto *minLabelPage = new MinLabelPage(shell);
    minLabelPage->setDataSource(dataSource, settingsBackend);
    shell->addPage(minLabelPage, "minlabel", {}, QObject::tr("MinLabel"));

    auto *phonemePage = new PhonemeLabelerPage(shell);
    phonemePage->setDataSource(dataSource, settingsBackend);
    shell->addPage(phonemePage, "phoneme", {}, QObject::tr("Phoneme"));

    auto *pitchPage = new PitchLabelerPage(shell);
    pitchPage->setDataSource(dataSource, settingsBackend);
    shell->addPage(pitchPage, "pitch", {}, QObject::tr("Pitch"));
}

SettingsPage *PageFactory::registerUtilityPages(
    dsfw::AppShell *shell,
    AppSettingsBackend *settingsBackend)
{
    auto *settingsPage = new SettingsPage(settingsBackend, shell);
    shell->addPage(settingsPage, "settings", {}, QObject::tr("Settings"));

    auto *logPage = new LogPage(shell);
    shell->addPage(logPage, "log", {}, QObject::tr("Log"));

    return settingsPage;
}

} // namespace dstools