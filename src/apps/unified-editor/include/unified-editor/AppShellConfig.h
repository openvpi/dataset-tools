#pragma once
/// @file AppShellConfig.h
/// @brief Shared AppShell configuration utilities for unified editor applications.
/// @details Extracts common AppShell setup patterns from ds-labeler and label-suite main.cpp
///          to reduce code duplication. Provides default window sizing, crash recovery dialogs,
///          and common initialization helpers.

#include <QMessageBox>
#include <QObject>
#include <QString>

#include <dsfw/AppPaths.h>
#include <dsfw/AppShell.h>
#include <dstools/AppInit.h>
#include <dstools/ModelManager.h>

namespace dstools::unified_editor {

    /// @brief Default window dimensions for editor applications.
    struct WindowDefaults {
        static constexpr int kWidth = 1400;
        static constexpr int kHeight = 900;
    };

    /// @brief Setup common AppShell properties (window title, size).
    /// @param shell The AppShell instance.
    /// @param title Window title.
    inline void setupShell(dsfw::AppShell &shell, const QString &title) {
        shell.setWindowTitle(title);
        shell.resize(WindowDefaults::kWidth, WindowDefaults::kHeight);
    }

    /// @brief Show crash recovery dialog if the previous session crashed.
    /// @param shell The AppShell instance (used as parent).
    inline void showCrashRecoveryDialog(dsfw::AppShell *shell) {
        if (!dstools::AppInit::wasPreviousCrash())
            return;

        QMessageBox::information(
            shell, QObject::tr("Abnormal Exit"),
            QObject::tr("The previous session ended unexpectedly (crash or forced termination).\n\n"
                        "Your annotation data is protected by auto-save.\n"
                        "Log files are saved in %1/logs for troubleshooting.")
                .arg(dsfw::AppPaths::dataDir()),
            QMessageBox::Ok);
    }

    /// @brief Connect model manager reload to settings page.
    /// @param shell The AppShell instance (used to find the SettingsPage child).
    /// @param modelManager The ModelManager instance.
    template <typename SettingsPageType>
    inline void connectModelReload(dsfw::AppShell *shell, dstools::ModelManager *modelManager) {
        if (!modelManager)
            return;
        auto *settingsPage = shell->findChild<SettingsPageType *>(QString(), Qt::FindDirectChildrenOnly);
        if (settingsPage) {
            QObject::connect(settingsPage, &SettingsPageType::modelReloadRequested, modelManager,
                             &dstools::ModelManager::invalidateModel);
        }
    }

} // namespace dstools::unified_editor