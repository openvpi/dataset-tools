#pragma once

#include <QMainWindow>

namespace dstools {
namespace pitchlabeler {

class PitchLabelerPage;

/// Thin shell window: delegates menus, status bar, and title to PitchLabelerPage.
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

    void updateWindowTitle();

signals:
    void fileLoaded(const QString &path);
    void fileSaved(const QString &path);
};

} // namespace pitchlabeler
} // namespace dstools
