#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHBoxLayout>
#include <QMainWindow>
#include <QTabWidget>

#include <some-infer/Some.h>

#include "inc/SomeCfg.h"

#include "MidiWidget.h"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    QHBoxLayout *mainLayout;
    QTabWidget *parentWidget;
    MidiWidget *midiWidget;

    SomeCfg cfg;

    std::shared_ptr<Some::Some> m_some;
};

#endif // MAINWINDOW_H
