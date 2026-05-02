#include "WelcomePage.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "NewProjectDialog.h"

namespace dstools {

static constexpr int kMaxRecentProjects = 10;
static const QString kRecentProjectsKey = QStringLiteral("App/recentProjects");

WelcomePage::WelcomePage(QWidget *parent) : QWidget(parent) {
    // Title
    m_titleLabel = new QLabel(QStringLiteral("DsLabeler"), this);
    auto titleFont = m_titleLabel->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    m_titleLabel->setAlignment(Qt::AlignCenter);

    // Subtitle
    m_subtitleLabel =
        new QLabel(QStringLiteral("DiffSinger 数据集标注工具"), this);
    auto subFont = m_subtitleLabel->font();
    subFont.setPointSize(12);
    m_subtitleLabel->setFont(subFont);
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    m_subtitleLabel->setStyleSheet(QStringLiteral("color: gray;"));

    // Buttons
    m_btnNewProject = new QPushButton(QStringLiteral("📁 新建工程"), this);
    m_btnOpenProject = new QPushButton(QStringLiteral("📂 打开工程"), this);

    static constexpr int kButtonWidth = 180;
    static constexpr int kButtonHeight = 48;
    m_btnNewProject->setMinimumSize(kButtonWidth, kButtonHeight);
    m_btnOpenProject->setMinimumSize(kButtonWidth, kButtonHeight);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnNewProject);
    btnLayout->addSpacing(20);
    btnLayout->addWidget(m_btnOpenProject);
    btnLayout->addStretch();

    // Recent projects
    auto *recentLabel = new QLabel(QStringLiteral("── 最近工程 ──"), this);
    recentLabel->setAlignment(Qt::AlignCenter);

    m_recentList = new QListWidget(this);
    m_recentList->setMaximumWidth(600);
    m_recentList->setAlternatingRowColors(true);

    // Layout
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch(2);
    mainLayout->addWidget(m_titleLabel);
    mainLayout->addWidget(m_subtitleLabel);
    mainLayout->addSpacing(30);
    mainLayout->addLayout(btnLayout);
    mainLayout->addSpacing(30);
    mainLayout->addWidget(recentLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(m_recentList, 0, Qt::AlignCenter);
    mainLayout->addStretch(3);

    // Signals
    connect(m_btnNewProject, &QPushButton::clicked, this, &WelcomePage::onNewProject);
    connect(m_btnOpenProject, &QPushButton::clicked, this, &WelcomePage::onOpenProject);
    connect(m_recentList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        loadProject(item->data(Qt::UserRole).toString());
    });

    refreshRecentList();
}

void WelcomePage::onActivated() {
    refreshRecentList();
}

QString WelcomePage::windowTitle() const {
    return QStringLiteral("DsLabeler");
}

QMenuBar *WelcomePage::createMenuBar(QWidget *parent) {
    auto *bar = new QMenuBar(parent);

    auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));
    fileMenu->addAction(QStringLiteral("新建工程(&N)..."), this, &WelcomePage::onNewProject);
    fileMenu->addAction(QStringLiteral("打开工程(&O)..."), this, &WelcomePage::onOpenProject);
    fileMenu->addSeparator();
    fileMenu->addAction(QStringLiteral("退出(&X)"), this, [this]() {
        if (auto *w = window())
            w->close();
    });

    auto *helpMenu = bar->addMenu(QStringLiteral("帮助(&H)"));
    helpMenu->addAction(QStringLiteral("关于(&A)"), this, []() {
        QMessageBox::about(nullptr, QStringLiteral("关于 DsLabeler"),
                           QStringLiteral("DsLabeler — DiffSinger Dataset Labeler\n"
                                          "版本 0.1.0\n\n"
                                          "Copyright Team OpenVPI."));
    });

    return bar;
}

void WelcomePage::onNewProject() {
    NewProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString projectPath = dlg.projectFilePath();
    if (projectPath.isEmpty())
        return;

    addToRecent(projectPath);
    loadProject(projectPath);
}

void WelcomePage::onOpenProject() {
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开工程"), QString(),
        QStringLiteral("DiffSinger Project (*.dsproj)"));
    if (path.isEmpty())
        return;

    addToRecent(path);
    loadProject(path);
}

void WelcomePage::loadProject(const QString &path) {
    QString error;
    auto *project = new DsProject(DsProject::load(path, error));
    if (!error.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("打开工程"),
                             QStringLiteral("加载工程失败:\n%1").arg(error));
        delete project;
        return;
    }

    addToRecent(path);
    emit projectLoaded(project, path);
}

void WelcomePage::addToRecent(const QString &path) {
    QSettings settings;
    auto recent = settings.value(kRecentProjectsKey).toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > kMaxRecentProjects)
        recent.removeLast();
    settings.setValue(kRecentProjectsKey, recent);
    refreshRecentList();
}

void WelcomePage::refreshRecentList() {
    m_recentList->clear();
    QSettings settings;
    const auto recent = settings.value(kRecentProjectsKey).toStringList();

    for (const auto &path : recent) {
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(
            QStringLiteral("%1    %2    %3")
                .arg(fi.fileName(), fi.lastModified().toString("yyyy-MM-dd"),
                     fi.absolutePath()));
        item->setData(Qt::UserRole, path);
        m_recentList->addItem(item);
    }
}

} // namespace dstools
