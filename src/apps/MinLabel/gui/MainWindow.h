#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Minlabel {

    class MinLabelPage;

    class MainWindow final : public QMainWindow {
        Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow() override;

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void closeEvent(QCloseEvent *event) override;

    private:
        MinLabelPage *m_page = nullptr;

        void buildMenuBar();
        void updateWindowTitle();
    };

} // namespace Minlabel

#endif // MAINWINDOW_H
