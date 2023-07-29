#include "mainwindow_ui.h"

void Ui_MainWindow::setupUi(QMainWindow *MainWindow) {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName("MainWindow");
    QSize s;
    MainWindow->resize(1920, 1080);

    centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName("centralwidget");

    verticalLayout = new QVBoxLayout(centralwidget);
    verticalLayout->setObjectName("verticalLayout");

    // Add Audio Files Button
    btnAddFiles = new QPushButton(centralwidget);
    btnAddFiles->setObjectName("btnAddFiles");
    btnAddFiles->setEnabled(true);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(btnAddFiles->sizePolicy().hasHeightForWidth());
    btnAddFiles->setSizePolicy(sizePolicy);

    hBoxAddFiles = new QHBoxLayout();
    hBoxAddFiles->setObjectName("hBoxAddFiles");
    hBoxAddFiles->addWidget(btnAddFiles);

    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hBoxAddFiles->addItem(horizontalSpacer);

    verticalLayout->addLayout(hBoxAddFiles);

    splitterLogs = new QSplitter(Qt::Vertical, centralwidget);
    splitterLogs->setObjectName("splitterLogs");

    // Main Area
    //hBoxMain = new QHBoxLayout();
    //hBoxMain->setObjectName("hBoxMain");
    splitterMain = new QSplitter(Qt::Horizontal, centralwidget);
    splitterMain->setObjectName("splitterMain");

    // Task List Group Box (left)
    gBoxTaskList = new QGroupBox(centralwidget);
    gBoxTaskList->setObjectName("gBoxTaskList");
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Minimum);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(gBoxTaskList->sizePolicy().hasHeightForWidth());
    gBoxTaskList->setSizePolicy(sizePolicy1);
    verticalLayout_2 = new QVBoxLayout(gBoxTaskList);
    verticalLayout_2->setObjectName("verticalLayout_2");
    listWidgetTaskList = new QListWidget(gBoxTaskList);
    listWidgetTaskList->setObjectName("listWidgetTaskList");
    listWidgetTaskList->setFrameShadow(QFrame::Plain);

    verticalLayout_2->addWidget(listWidgetTaskList);

    btnClearList = new QPushButton(gBoxTaskList);
    btnClearList->setObjectName("btnClearList");

    verticalLayout_2->addWidget(btnClearList);

    splitterMain->addWidget(gBoxTaskList);
    //hBoxMain->addWidget(gBoxTaskList);

    // Settings Group Box (right)
    gBoxSettings = new QGroupBox(centralwidget);
    gBoxSettings->setObjectName("gBoxSettings");
    sizePolicy1.setHeightForWidth(gBoxSettings->sizePolicy().hasHeightForWidth());
    gBoxSettings->setSizePolicy(sizePolicy1);
    verticalLayout_3 = new QVBoxLayout(gBoxSettings);
    verticalLayout_3->setObjectName("verticalLayout_3");
    formLayout = new QFormLayout();
    formLayout->setObjectName("formLayout");
    lblThreshold = new QLabel(gBoxSettings);
    lblThreshold->setObjectName("lblThreshold");

    formLayout->setWidget(0, QFormLayout::LabelRole, lblThreshold);

    lineEditThreshold = new QLineEdit(gBoxSettings);
    lineEditThreshold->setObjectName("lineEditThreshold");
    lineEditThreshold->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(0, QFormLayout::FieldRole, lineEditThreshold);

    lblMinLen = new QLabel(gBoxSettings);
    lblMinLen->setObjectName("lblMinLen");

    formLayout->setWidget(1, QFormLayout::LabelRole, lblMinLen);

    lineEditMinLen = new QLineEdit(gBoxSettings);
    lineEditMinLen->setObjectName("lineEditMinLen");
    lineEditMinLen->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(1, QFormLayout::FieldRole, lineEditMinLen);

    lblMinInterval = new QLabel(gBoxSettings);
    lblMinInterval->setObjectName("lblMinInterval");

    formLayout->setWidget(2, QFormLayout::LabelRole, lblMinInterval);

    lineEditMinInterval = new QLineEdit(gBoxSettings);
    lineEditMinInterval->setObjectName("lineEditMinInterval");
    lineEditMinInterval->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(2, QFormLayout::FieldRole, lineEditMinInterval);

    lblHopSize = new QLabel(gBoxSettings);
    lblHopSize->setObjectName("lblHopSize");

    formLayout->setWidget(3, QFormLayout::LabelRole, lblHopSize);

    lineEditHopSize = new QLineEdit(gBoxSettings);
    lineEditHopSize->setObjectName("lineEditHopSize");
    lineEditHopSize->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(3, QFormLayout::FieldRole, lineEditHopSize);

    lblMaxSilence = new QLabel(gBoxSettings);
    lblMaxSilence->setObjectName("lblMaxSilence");

    formLayout->setWidget(4, QFormLayout::LabelRole, lblMaxSilence);

    lineEditMaxSilence = new QLineEdit(gBoxSettings);
    lineEditMaxSilence->setObjectName("lineEditMaxSilence");
    lineEditMaxSilence->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(4, QFormLayout::FieldRole, lineEditMaxSilence);


    verticalLayout_3->addLayout(formLayout);

    lblOutputDir = new QLabel(gBoxSettings);
    lblOutputDir->setObjectName("lblOutputDir");

    verticalLayout_3->addWidget(lblOutputDir);

    hBoxOutputDir = new QHBoxLayout();
    hBoxOutputDir->setObjectName("hBoxOutputDir");
    lineEditOutputDir = new QLineEdit(gBoxSettings);
    lineEditOutputDir->setObjectName("lineEditOutputDir");
    lineEditOutputDir->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

    hBoxOutputDir->addWidget(lineEditOutputDir);

    btnBrowse = new QPushButton(gBoxSettings);
    btnBrowse->setObjectName("btnBrowse");

    hBoxOutputDir->addWidget(btnBrowse);

    verticalLayout_3->addLayout(hBoxOutputDir);

    lblOutputWaveFormat = new QLabel(gBoxSettings);
    lblOutputWaveFormat->setObjectName("lblOutputWaveFormat");
    verticalLayout_3->addWidget(lblOutputWaveFormat);

    cmbOutputWaveFormat = new QComboBox(gBoxSettings);
    cmbOutputWaveFormat->setObjectName("cmbOutputWaveFormat");
    verticalLayout_3->addWidget(cmbOutputWaveFormat);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_3->addItem(verticalSpacer);

    splitterMain->addWidget(gBoxSettings);
    //hBoxMain->addWidget(gBoxSettings);

    //hBoxMain->setStretch(0, 4);
    //hBoxMain->setStretch(1, 3);

    //verticalLayout->addLayout(hBoxMain);
    //verticalLayout->addWidget(splitterMain);
    splitterLogs->addWidget(splitterMain);

    txtLogs = new QTextEdit(centralwidget);
    txtLogs->setObjectName("txtLogs");
    txtLogs->setReadOnly(true);

    //verticalLayout->addWidget(txtLogs);
    splitterLogs->addWidget(txtLogs);
    verticalLayout->addWidget(splitterLogs);

    hBoxBottom = new QHBoxLayout();
    hBoxBottom->setObjectName("hBoxBottom");
    pushButtonAbout = new QPushButton(centralwidget);
    pushButtonAbout->setObjectName("pushButtonAbout");

    hBoxBottom->addWidget(pushButtonAbout);

    progressBar = new QProgressBar(centralwidget);
    progressBar->setObjectName("progressBar");
    progressBar->setValue(0);

    hBoxBottom->addWidget(progressBar);

    pushButtonStart = new QPushButton(centralwidget);
    pushButtonStart->setObjectName("pushButtonStart");

    hBoxBottom->addWidget(pushButtonStart);


    verticalLayout->addLayout(hBoxBottom);

    verticalLayout->setStretch(1, 3);
    verticalLayout->setStretch(2, 1);
    MainWindow->setCentralWidget(centralwidget);

    // Menu Bar
    menuBar = new QMenuBar(MainWindow);
    menuBar->setObjectName("menuBar");
    menuBar->setGeometry(QRect(0, 0, 800, 22));
    menuFile = new QMenu(menuBar);
    menuFile->setObjectName("menuFile");
    menuHelp = new QMenu(menuBar);
    menuHelp->setObjectName("menuHelp");
    // menuView = new QMenu(menuBar);
    // menuView->setObjectName("menuView");
    MainWindow->setMenuBar(menuBar);

    actionAddFile = new QAction(MainWindow);
    actionAddFile->setObjectName("actionAddFile");
    actionAddFolder = new QAction(MainWindow);
    actionAddFolder->setObjectName("actionAddFolder");
    actionSaveLogs = new QAction(MainWindow);
    actionSaveLogs->setObjectName("actionSaveLogs");
    actionQuit = new QAction(MainWindow);
    actionQuit->setObjectName("actionQuit");
    actionAboutApp = new QAction(MainWindow);
    actionAboutApp->setObjectName("actionAboutApp");
    actionAboutQt = new QAction(MainWindow);
    actionAboutQt->setObjectName("actionAboutQt");
    // actionShowHideLogs = new QAction(MainWindow);
    // actionShowHideLogs->setObjectName("actionShowHideLogs");

    menuBar->addAction(menuFile->menuAction());
    // menuBar->addAction(menuView->menuAction());
    menuBar->addAction(menuHelp->menuAction());
    menuFile->addAction(actionAddFile);
    menuFile->addAction(actionAddFolder);
    menuFile->addSeparator();
    menuFile->addAction(actionSaveLogs);
    menuFile->addSeparator();
    menuFile->addAction(actionQuit);
    menuHelp->addAction(actionAboutApp);
    menuHelp->addAction(actionAboutQt);
    // menuView->addAction(actionShowHideLogs);

    splitterMain->setSizes({1024, 80});
    splitterMain->setCollapsible(0, false);
    splitterMain->setCollapsible(1, false);
    splitterLogs->setCollapsible(0, false);
    splitterLogs->setCollapsible(1, true);
    retranslateUi(MainWindow);
}
// setupUi

void Ui_MainWindow::retranslateUi(QMainWindow *MainWindow)
{
    MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
    actionAddFile->setText(QCoreApplication::translate("MainWindow", "Add Files...", nullptr));
    actionAddFolder->setText(QCoreApplication::translate("MainWindow", "Add &Folder...", nullptr));
    actionSaveLogs->setText(QCoreApplication::translate("MainWindow", "Save Logs", nullptr));
    actionQuit->setText(QCoreApplication::translate("MainWindow", "&Quit", nullptr));
    actionAboutApp->setText(QCoreApplication::translate("MainWindow", "&About Audio Slicer", nullptr));
    actionAboutQt->setText(QCoreApplication::translate("MainWindow", "About &Qt", nullptr));
    // actionShowHideLogs->setText(QCoreApplication::translate("MainWindow", "Show/Hide &Logs", nullptr));
    btnAddFiles->setText(QCoreApplication::translate("MainWindow", "Add Audio Files...", nullptr));
    gBoxTaskList->setTitle(QCoreApplication::translate("MainWindow", "Task List", nullptr));
    btnClearList->setText(QCoreApplication::translate("MainWindow", "Clear List", nullptr));
    gBoxSettings->setTitle(QCoreApplication::translate("MainWindow", "Settings", nullptr));
    lblThreshold->setText(QCoreApplication::translate("MainWindow", "Threshold (dB)", nullptr));
    lineEditThreshold->setText(QCoreApplication::translate("MainWindow", "-40", nullptr));
    lblMinLen->setText(QCoreApplication::translate("MainWindow", "Minimum Length (ms)", nullptr));
    lineEditMinLen->setText(QCoreApplication::translate("MainWindow", "5000", nullptr));
    lblMinInterval->setText(QCoreApplication::translate("MainWindow", "Minimum Interval (ms)", nullptr));
    lineEditMinInterval->setText(QCoreApplication::translate("MainWindow", "300", nullptr));
    lblHopSize->setText(QCoreApplication::translate("MainWindow", "Hop Size (ms)", nullptr));
    lineEditHopSize->setText(QCoreApplication::translate("MainWindow", "10", nullptr));
    lblMaxSilence->setText(QCoreApplication::translate("MainWindow", "Maximum Silence Length (ms)", nullptr));
    lineEditMaxSilence->setText(QCoreApplication::translate("MainWindow", "1000", nullptr));
    lblOutputDir->setText(QCoreApplication::translate("MainWindow", "Output Directory (default to the same as the audio)", nullptr));
    lineEditOutputDir->setText(QString());
    lblOutputWaveFormat->setText(QCoreApplication::translate("MainWindow", "Output Wave Format", nullptr));
    btnBrowse->setText(QCoreApplication::translate("MainWindow", "Browse...", nullptr));
    pushButtonAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
    pushButtonStart->setText(QCoreApplication::translate("MainWindow", "Start", nullptr));
    menuFile->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
    menuHelp->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
    // menuView->setTitle(QCoreApplication::translate("MainWindow", "&View", nullptr));
} // retranslateUi