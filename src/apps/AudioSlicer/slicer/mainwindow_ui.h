#ifndef AUDIO_SLICER_UI_MAINWINDOW_H
#define AUDIO_SLICER_UI_MAINWINDOW_H

#include <QVariant>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QProgressBar>
#include <QPushButton>
#include <QSpacerItem>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionAddFile, *actionAddFolder, *actionSaveLogs, *actionQuit;
    //QAction *actionShowHideLogs;
    QAction *actionAboutApp, *actionAboutQt;
    QMenuBar *menuBar;
    QMenu *menuFile, *menuHelp; //, *menuView;

    QWidget *centralwidget;
    QSplitter *splitterMain, *splitterLogs;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *hBoxAddFiles;
    QPushButton *btnAddFiles;
    QSpacerItem *horizontalSpacer;
    //QHBoxLayout *hBoxMain;
    QGroupBox *gBoxTaskList;
    QVBoxLayout *verticalLayout_2;
    QListWidget *listWidgetTaskList;
    QPushButton *btnClearList;
    QGroupBox *gBoxSettings;
    QVBoxLayout *verticalLayout_3;
    QFormLayout *formLayout;
    QLabel *lblThreshold;
    QLineEdit *lineEditThreshold;
    QLabel *lblMinLen;
    QLineEdit *lineEditMinLen;
    QLabel *lblMinInterval;
    QLineEdit *lineEditMinInterval;
    QLabel *lblHopSize;
    QLineEdit *lineEditHopSize;
    QLabel *lblMaxSilence;
    QLineEdit *lineEditMaxSilence;
    QLabel *lblOutputDir;
    QHBoxLayout *hBoxOutputDir;
    QLineEdit *lineEditOutputDir;
    QPushButton *btnBrowse;
    QSpacerItem *verticalSpacer;
    QTextEdit *txtLogs;
    QHBoxLayout *hBoxBottom;
    QPushButton *pushButtonAbout;
    QProgressBar *progressBar;
    QPushButton *pushButtonStart;
    QLabel *lblOutputWaveFormat;
    QComboBox *cmbOutputWaveFormat;

    void setupUi(QMainWindow *MainWindow);
    void retranslateUi(QMainWindow *MainWindow);

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // AUDIO_SLICER_UI_MAINWINDOW_H
