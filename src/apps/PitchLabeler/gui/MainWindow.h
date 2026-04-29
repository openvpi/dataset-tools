#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QLabel>

namespace dstools {
namespace pitchlabeler {

class PitchLabelerPage;

/// Thin shell window: menus, status bar, window chrome.
/// All editor logic lives in PitchLabelerPage.
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    PitchLabelerPage *m_page = nullptr;

    // Menus
    QMenu *m_fileMenu = nullptr;
    QMenu *m_editMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_toolsMenu = nullptr;
    QMenu *m_helpMenu = nullptr;

    // Menu actions owned by MainWindow
    QAction *m_actOpenDir = nullptr;
    QAction *m_actCloseDir = nullptr;
    QAction *m_actExit = nullptr;
    QAction *m_actAbout = nullptr;

    // Status bar
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusPosition = nullptr;
    QLabel *m_statusZoom = nullptr;
    QLabel *m_statusNotes = nullptr;
    QLabel *m_statusTool = nullptr;

    bool m_fileLoaded = false;

    void buildMenuBar();
    void buildStatusBar();
    void updateWindowTitle();

    // File operations owned by MainWindow
    void openDirectory();

signals:
    void fileLoaded(const QString &path);
    void fileSaved(const QString &path);
};

} // namespace pitchlabeler
} // namespace dstools
