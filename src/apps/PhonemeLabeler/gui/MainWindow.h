#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QLabel>

namespace dstools {
namespace phonemelabeler {

class PhonemeLabelerPage;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    PhonemeLabelerPage *m_page = nullptr;

    // Menus
    QMenu *m_fileMenu = nullptr;
    QMenu *m_editMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_helpMenu = nullptr;

    // Status bar labels
    QLabel *m_statusFile = nullptr;
    QLabel *m_statusPosition = nullptr;
    QLabel *m_statusZoom = nullptr;
    QLabel *m_statusBinding = nullptr;

    void buildMenuBar();
    void buildStatusBar();
    void updateWindowTitle();
};

} // namespace phonemelabeler
} // namespace dstools
