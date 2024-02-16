#include "mainwindow_ui.h"
#include "enumerations.h"

void Ui_MainWindow::setupUi(QMainWindow *MainWindow) {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName("MainWindow");
    QSize s;
    MainWindow->resize(800, 600);

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
    listWidgetTaskList->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    verticalLayout_2->addWidget(listWidgetTaskList);

    hBoxListButtons = new QHBoxLayout();
    hBoxListButtons->setObjectName("hBoxListButtons");

    btnRemoveListItem = new QPushButton(gBoxTaskList);
    btnRemoveListItem->setObjectName("btnRemoveListItem");

    btnClearList = new QPushButton(gBoxTaskList);
    btnClearList->setObjectName("btnClearList");

    hBoxListButtons->addWidget(btnRemoveListItem);
    hBoxListButtons->addWidget(btnClearList);

    verticalLayout_2->addLayout(hBoxListButtons);

    splitterMain->addWidget(gBoxTaskList);
    //hBoxMain->addWidget(gBoxTaskList);

    // Settings Group Box (right)
    gBoxSettings = new QScrollArea(centralwidget);
    gBoxSettings->setObjectName("gBoxSettings");
    gBoxSettings->setWidgetResizable(true);
    //sizePolicy1.setHeightForWidth(gBoxSettings->sizePolicy().hasHeightForWidth());
    //gBoxSettings->setSizePolicy(sizePolicy1);

    gBoxSettings->setFrameShape(QFrame::NoFrame);
    gBoxSettings->setFrameShadow(QFrame::Plain);
    gBoxSettings->setLineWidth(0);
    gBoxSettings->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    settingsContainer = new QWidget(gBoxSettings);
    settingsContainer->setObjectName("settingsContainer");
    vlSettingsArea = new QVBoxLayout(gBoxSettings);
    vlSettingsArea->setObjectName("vlSettingsArea");
    vlSettingsArea->setContentsMargins(0, 0, 0, 0);
    settingsContainer->setLayout(vlSettingsArea);
    gBoxSettings->setWidget(settingsContainer);

    // Parameters
    gBoxParameters = new QGroupBox(gBoxSettings);
    gBoxParameters->setObjectName("gBoxParameters");

    formLayout = new QFormLayout();
    formLayout->setObjectName("formLayout");
    lblThreshold = new QLabel(gBoxParameters);
    lblThreshold->setObjectName("lblThreshold");

    formLayout->setWidget(0, QFormLayout::LabelRole, lblThreshold);

    lineEditThreshold = new QLineEdit(gBoxParameters);
    lineEditThreshold->setObjectName("lineEditThreshold");
    lineEditThreshold->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(0, QFormLayout::FieldRole, lineEditThreshold);

    lblMinLen = new QLabel(gBoxParameters);
    lblMinLen->setObjectName("lblMinLen");

    formLayout->setWidget(1, QFormLayout::LabelRole, lblMinLen);

    lineEditMinLen = new QLineEdit(gBoxParameters);
    lineEditMinLen->setObjectName("lineEditMinLen");
    lineEditMinLen->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(1, QFormLayout::FieldRole, lineEditMinLen);

    lblMinInterval = new QLabel(gBoxParameters);
    lblMinInterval->setObjectName("lblMinInterval");

    formLayout->setWidget(2, QFormLayout::LabelRole, lblMinInterval);

    lineEditMinInterval = new QLineEdit(gBoxParameters);
    lineEditMinInterval->setObjectName("lineEditMinInterval");
    lineEditMinInterval->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(2, QFormLayout::FieldRole, lineEditMinInterval);

    lblHopSize = new QLabel(gBoxParameters);
    lblHopSize->setObjectName("lblHopSize");

    formLayout->setWidget(3, QFormLayout::LabelRole, lblHopSize);

    lineEditHopSize = new QLineEdit(gBoxParameters);
    lineEditHopSize->setObjectName("lineEditHopSize");
    lineEditHopSize->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(3, QFormLayout::FieldRole, lineEditHopSize);

    lblMaxSilence = new QLabel(gBoxParameters);
    lblMaxSilence->setObjectName("lblMaxSilence");

    formLayout->setWidget(4, QFormLayout::LabelRole, lblMaxSilence);

    lineEditMaxSilence = new QLineEdit(gBoxParameters);
    lineEditMaxSilence->setObjectName("lineEditMaxSilence");
    lineEditMaxSilence->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    formLayout->setWidget(4, QFormLayout::FieldRole, lineEditMaxSilence);

    gBoxParameters->setLayout(formLayout);
    vlSettingsArea->addWidget(gBoxParameters);

    // Audio Options
    gBoxAudioOptions = new QGroupBox(gBoxSettings);
    gBoxAudioOptions->setObjectName("gBoxAudioOptions");

    vlAudioOptions = new QVBoxLayout(gBoxSettings);
    vlAudioOptions->setObjectName("vlAudioOptions");

    lblOutputWaveFormat = new QLabel(gBoxAudioOptions);
    lblOutputWaveFormat->setObjectName("lblOutputWaveFormat");
    vlAudioOptions->addWidget(lblOutputWaveFormat);

    cmbOutputWaveFormat = new QComboBox(gBoxAudioOptions);
    cmbOutputWaveFormat->setObjectName("cmbOutputWaveFormat");
    vlAudioOptions->addWidget(cmbOutputWaveFormat);
    lblOutputDir = new QLabel(gBoxAudioOptions);
    lblOutputDir->setObjectName("lblOutputDir");

    vlAudioOptions->addWidget(lblOutputDir);

    hBoxOutputDir = new QHBoxLayout();
    hBoxOutputDir->setObjectName("hBoxOutputDir");
    lineEditOutputDir = new QLineEdit(gBoxAudioOptions);
    lineEditOutputDir->setObjectName("lineEditOutputDir");
    lineEditOutputDir->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

    hBoxOutputDir->addWidget(lineEditOutputDir);

    btnBrowse = new QPushButton(gBoxAudioOptions);
    btnBrowse->setObjectName("btnBrowse");

    hBoxOutputDir->addWidget(btnBrowse);

    vlAudioOptions->addLayout(hBoxOutputDir);

    gBoxAudioOptions->setLayout(vlAudioOptions);

    vlSettingsArea->addWidget(gBoxAudioOptions);

    // Output Filename
    gBoxFilename = new QGroupBox(gBoxSettings);
    gBoxFilename->setObjectName("gBoxFilename");

    vlFilename = new QVBoxLayout(gBoxFilename);
    vlFilename->setObjectName("vlFilename");

    hlSuffixDigits = new QHBoxLayout(gBoxFilename);
    hlSuffixDigits->setObjectName("hlSuffixDigits");

    lblSuffixDigits = new QLabel(gBoxFilename);
    lblSuffixDigits->setObjectName("lblSuffixDigits");

    spinBoxSuffixDigits = new QSpinBox(gBoxFilename);
    spinBoxSuffixDigits->setObjectName("spinBoxSuffixDigits");
    spinBoxSuffixDigits->setValue(3);
    spinBoxSuffixDigits->setRange(1, 20);

    hlSuffixDigits->addWidget(lblSuffixDigits);
    hlSuffixDigits->addWidget(spinBoxSuffixDigits);

    vlFilename->addLayout(hlSuffixDigits);

    lblFilenameExample = new QLabel(gBoxFilename);
    lblFilenameExample->setObjectName("lblFilenameExample");

    vlFilename->addWidget(lblFilenameExample);

    gBoxFilename->setLayout(vlFilename);

    vlSettingsArea->addWidget(gBoxFilename);

    // Slicing Mode
    gBoxSlicingMode = new QGroupBox(gBoxSettings);
    gBoxSlicingMode->setObjectName("gBoxSlicingMode");

    vlSlicingMode = new QVBoxLayout(gBoxSlicingMode);
    vlSlicingMode->setObjectName("vlSlicingMode");

    cmbSlicingMode = new QComboBox(gBoxSlicingMode);
    cmbSlicingMode->setObjectName("cmbSlicingMode");

    cmbSlicingMode->addItem(QString(), QVariant::fromValue(SlicingMode::AudioOnly));
    cmbSlicingMode->addItem(QString(), QVariant::fromValue(SlicingMode::AudioOnlyLoadMarkers));
    cmbSlicingMode->addItem(QString(), QVariant::fromValue(SlicingMode::MarkersOnly));
    cmbSlicingMode->addItem(QString(), QVariant::fromValue(SlicingMode::AudioAndMarkers));

    vlSlicingMode->addWidget(cmbSlicingMode);

    cbOverwriteMarkers = new QCheckBox(gBoxSlicingMode);
    cbOverwriteMarkers->setObjectName("cbOverwriteMarkers");
    vlSlicingMode->addWidget(cbOverwriteMarkers);

    gBoxSlicingMode->setLayout(vlSlicingMode);

    vlSettingsArea->addWidget(gBoxSlicingMode);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    vlSettingsArea->addItem(verticalSpacer);

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
    menuView = new QMenu(menuBar);
    menuView->setObjectName("menuView");
    menuHelp = new QMenu(menuBar);
    menuHelp->setObjectName("menuHelp");
    MainWindow->setMenuBar(menuBar);

    actionAddFile = new QAction(MainWindow);
    actionAddFile->setObjectName("actionAddFile");
    actionAddFolder = new QAction(MainWindow);
    actionAddFolder->setObjectName("actionAddFolder");
    actionSaveLogs = new QAction(MainWindow);
    actionSaveLogs->setObjectName("actionSaveLogs");
    actionQuit = new QAction(MainWindow);
    actionQuit->setObjectName("actionQuit");
    menuViewThemes = new QMenu(menuView);
    menuViewThemes->setObjectName("menuViewThemes");
    actionAboutApp = new QAction(MainWindow);
    actionAboutApp->setObjectName("actionAboutApp");
    actionAboutQt = new QAction(MainWindow);
    actionAboutQt->setObjectName("actionAboutQt");
    // actionShowHideLogs = new QAction(MainWindow);
    // actionShowHideLogs->setObjectName("actionShowHideLogs");
    actionGroupThemes = new QActionGroup(MainWindow);
    actionGroupThemes->setObjectName("actionGroupThemes");

    menuBar->addAction(menuFile->menuAction());
    menuBar->addAction(menuView->menuAction());
    menuBar->addAction(menuHelp->menuAction());
    menuFile->addAction(actionAddFile);
    menuFile->addAction(actionAddFolder);
    menuFile->addSeparator();
    menuFile->addAction(actionSaveLogs);
    menuFile->addSeparator();
    menuFile->addAction(actionQuit);
    menuView->addMenu(menuViewThemes);
#if defined(Q_OS_MAC)
    menuView->addSeparator();
#endif
    menuHelp->addAction(actionAboutApp);
    menuHelp->addAction(actionAboutQt);
    // menuView->addAction(actionShowHideLogs);

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
    btnRemoveListItem->setText(QCoreApplication::translate("MainWindow", "Remove", nullptr));
    btnClearList->setText(QCoreApplication::translate("MainWindow", "Clear List", nullptr));
    gBoxParameters->setTitle(QCoreApplication::translate("MainWindow", "Parameters", nullptr));
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
    gBoxAudioOptions->setTitle(QCoreApplication::translate("MainWindow", "Audio", nullptr));
    lblOutputDir->setText(QCoreApplication::translate("MainWindow", "Output Directory (default to source directory)", nullptr));
    lineEditOutputDir->setText(QString());
    lblOutputWaveFormat->setText(QCoreApplication::translate("MainWindow", "Output Wave Format", nullptr));
    gBoxFilename->setTitle(QCoreApplication::translate("MainWindow", "Output Filename", nullptr));
    lblSuffixDigits->setText(QCoreApplication::translate("MainWindow", "Minimum suffix digits", nullptr));
    gBoxSlicingMode->setTitle(QCoreApplication::translate("MainWindow", "Slicing Mode", nullptr));
    cmbSlicingMode->setItemText(0, QCoreApplication::translate("MainWindow", "Save audio chunks only", nullptr));
    cmbSlicingMode->setItemText(1, QCoreApplication::translate("MainWindow", "Save audio chunks only (load markers)", nullptr));
    cmbSlicingMode->setItemText(2, QCoreApplication::translate("MainWindow", "Save markers only", nullptr));
    cmbSlicingMode->setItemText(3, QCoreApplication::translate("MainWindow", "Save audio chunks and markers", nullptr));
    cbOverwriteMarkers->setText(QCoreApplication::translate("MainWindow", "Allow overwriting markers if already exist", nullptr));
    btnBrowse->setText(QCoreApplication::translate("MainWindow", "Browse...", nullptr));
    pushButtonAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
    pushButtonStart->setText(QCoreApplication::translate("MainWindow", "Start", nullptr));
    menuFile->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
    menuView->setTitle(QCoreApplication::translate("MainWindow", "&View", nullptr));
    menuViewThemes->setTitle(QCoreApplication::translate("MainWindow", "Themes", nullptr));
    menuHelp->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
} // retranslateUi