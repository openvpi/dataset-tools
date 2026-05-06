#pragma once

/// @file LogPanelWidget.h
/// @brief Collapsible side panel displaying structured log output with filtering.

#include <dsfw/Log.h>
#include <QComboBox>
#include <QPushButton>
#include <QSet>
#include <QWidget>

namespace dsfw::widgets {
class LogViewer;
}

namespace dsfw {

/// @brief Right-side log panel with level filter, category filter, clear, and export.
///
/// Connects to dstools::LogNotifier for live updates and prepopulates
/// from dstools::Logger::recentEntries() on construction.
class LogPanelWidget : public QWidget {
    Q_OBJECT

public:
    explicit LogPanelWidget(QWidget *parent = nullptr);

    /// @brief Connect to LogNotifier for live log updates.
    void connectToNotifier();

    /// @brief Append a log entry programmatically.
    void appendEntry(const dstools::LogEntry &entry);

    /// @brief Export all visible entries to a file.
    void exportToFile();

    /// @brief Clear all entries.
    void clear();

private:
    void rebuildCategoryFilter();
    void applyCategoryFilter();

    widgets::LogViewer *m_viewer = nullptr;
    QComboBox *m_categoryFilter = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QSet<QString> m_knownCategories;
    QSet<QString> m_activeCategories; // Empty = show all
};

} // namespace dsfw
