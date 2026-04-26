#pragma once

/// @file WorkspaceConfig.h
/// @brief Centralized workspace configuration persistence system.
///
/// Records sidebar proportions, icon sizes, and other layout state
/// so that window proportions are restored identically across different projects.

#include <QHash>
#include <QList>
#include <QString>

class QSplitter;

namespace dstools {
    class AppSettings;
}

namespace dsfw {

    /// @brief Manages global workspace layout configuration.
    ///
    /// Provides centralized save/restore for all splitter states, icon sizes,
    /// and other workspace layout parameters. State is stored in AppSettings
    /// under the "Workspace/" key prefix and is shared across all projects.
    ///
    /// Usage:
    /// @code
    /// WorkspaceConfig ws(settings);
    /// ws.setIconSize(24);
    /// ws.saveSplitterState("main", m_mainSplitter);
    /// ws.saveAll();
    /// @endcode
    class WorkspaceConfig {
    public:
        explicit WorkspaceConfig(dstools::AppSettings *settings);

        // Icon size
        int iconSize() const;
        void setIconSize(int size);

        // Splitter state management
        void saveSplitterState(const QString &key, QSplitter *splitter);
        void restoreSplitterState(const QString &key, QSplitter *splitter);

        // Sidebar collapsed state
        void saveSidebarCollapsed(const QString &key, bool collapsed);
        bool restoreSidebarCollapsed(const QString &key, bool defaultVal = false) const;

        // Bulk save/restore
        void saveAll();
        void restoreAll();

    private:
        dstools::AppSettings *m_settings = nullptr;
    };

} // namespace dsfw