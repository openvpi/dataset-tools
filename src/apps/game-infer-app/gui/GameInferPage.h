/// @file GameInferPage.h
/// @brief GameInfer application main page widget.

#pragma once

#include <dsfw/IPageActions.h>

#include <QWidget>

class QProgressBar;
class MainWidget;

namespace dstools {
class AppSettings;
}

/// @brief IPageActions page for the GameInfer standalone application.
class GameInferPage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    /// @param settings Application settings instance.
    /// @param parent Optional parent widget.
    explicit GameInferPage(dstools::AppSettings *settings, QWidget *parent = nullptr);

    // IPageActions
    /// @brief Create the menu bar for this page.
    QMenuBar *createMenuBar(QWidget *parent) override;
    /// @brief Create status bar content for this page.
    QWidget *createStatusBarContent(QWidget *parent) override;
    /// @brief Return the window title for this page.
    QString windowTitle() const override;
    /// @brief Whether this page supports drag-and-drop.
    bool supportsDragDrop() const override;
    /// @brief Handle drag enter events.
    void handleDragEnter(QDragEnterEvent *event) override;
    /// @brief Handle drop events.
    void handleDrop(QDropEvent *event) override;

private:
    MainWidget *m_mainWidget;          ///< Main control panel widget.
    dstools::AppSettings *m_settings;  ///< Application settings.
};
