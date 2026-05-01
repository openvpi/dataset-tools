/// @file PipelinePage.h
/// @brief DatasetPipeline multi-tab page container.

#pragma once

#include <dsfw/IPageActions.h>

#include <QWidget>

class QTabWidget;
class SlicerPage;
class LyricFAPage;
class HubertFAPage;

namespace dstools {
class AppSettings;
}

/// @brief IPageActions page that hosts Slicer, LyricFA, and HubertFA tabs.
class PipelinePage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    /// @param settings Application settings instance.
    /// @param parent Optional parent widget.
    explicit PipelinePage(dstools::AppSettings *settings, QWidget *parent = nullptr);

    // IPageActions
    /// @brief Create the menu bar for this page.
    QMenuBar *createMenuBar(QWidget *parent) override;
    /// @brief Create status bar content for this page.
    QWidget *createStatusBarContent(QWidget *parent) override;
    /// @brief Return the window title for this page.
    QString windowTitle() const override;

private:
    QTabWidget *m_tabWidget;       ///< Tab widget hosting pipeline pages.
    SlicerPage *m_slicerPage;      ///< Audio slicer tab.
    LyricFAPage *m_lyricFAPage;    ///< LyricFA tab.
    HubertFAPage *m_hubertFAPage;  ///< HubertFA tab.

    dstools::AppSettings *m_settings; ///< Application settings.
};
