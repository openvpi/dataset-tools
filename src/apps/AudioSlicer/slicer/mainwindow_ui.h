#ifndef AUDIO_SLICER_UI_MAINWINDOW_H
#define AUDIO_SLICER_UI_MAINWINDOW_H

#include <QVariant>
#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCheckBox>
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
#include <QScrollArea>
#include <QScrollBar>
#include <QSpacerItem>
#include <QSplitter>
#include <QSpinBox>
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
    QActionGroup *actionGroupThemes;
    QMenuBar *menuBar;
    QMenu *menuFile, *menuView, *menuHelp;
    QMenu *menuViewThemes;

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
    QHBoxLayout *hBoxListButtons;
    QPushButton *btnRemoveListItem;
    QPushButton *btnClearList;
    QScrollArea *gBoxSettings;
    QWidget *settingsContainer;
    QVBoxLayout *vlSettingsArea;
    QGroupBox *gBoxParameters, *gBoxAudioOptions, *gBoxFilename, *gBoxSlicingMode;
    QFormLayout *formLayout;
    QLabel *lblThreshold;
    QLineEdit *lineEditThreshold;
    QLabel *lblMaxLen;
    QLineEdit *lineEditMaxLen;
    QLabel *lblMinInterval;
    QLineEdit *lineEditMinInterval;
    QLabel *lblHopSize;
    QLineEdit *lineEditHopSize;
    QLabel *lblMaxSilence;
    QLineEdit *lineEditMaxSilence;
    QVBoxLayout *vlAudioOptions;
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
    QVBoxLayout *vlSlicingMode;
    QComboBox *cmbSlicingMode;
    QCheckBox *cbOverwriteMarkers;
    QVBoxLayout *vlFilename;
    QHBoxLayout *hlSuffixDigits;
    QLabel *lblSuffixDigits;
    QSpinBox *spinBoxSuffixDigits;
    QLabel *lblFilenameExample;

    void setupUi(QMainWindow *MainWindow);
    void retranslateUi(QMainWindow *MainWindow);

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // AUDIO_SLICER_UI_MAINWINDOW_H
