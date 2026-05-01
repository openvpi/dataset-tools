#pragma once

#include <dsfw/IAlignmentService.h>

#include <QWidget>

class QLineEdit;
class QComboBox;
class ProgressWidget;

class GameAlignPage : public QWidget {
    Q_OBJECT
public:
    explicit GameAlignPage(QWidget *parent = nullptr);

private slots:
    void onRunAlignment();

private:
    QLineEdit *m_modelPath;
    QComboBox *m_gpuSelector;
    ProgressWidget *m_runProgress;
    QWidget *m_log;
};
