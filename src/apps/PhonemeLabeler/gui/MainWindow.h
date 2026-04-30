#pragma once

#include <QMainWindow>

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

    void buildMenuBar();
    void buildStatusBar();
    void updateWindowTitle();
};

} // namespace phonemelabeler
} // namespace dstools
