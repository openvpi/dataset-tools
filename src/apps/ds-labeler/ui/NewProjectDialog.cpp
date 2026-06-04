#include "NewProjectDialog.h"

#include "Keys.h"

#include <dsfw/AppSettings.h>
#include <dstools/DsProject.h>

#include <dsfw/widgets/FilePathSelector.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace dstools {

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(QStringLiteral("新建工程"));
    setMinimumSize(500, 450);

    // --- Project info ---
    auto *infoGroup = new QGroupBox(QStringLiteral("基本信息"), this);
    auto *infoLayout = new QFormLayout(infoGroup);

    m_editName = new QLineEdit(this);
    m_editName->setPlaceholderText(QStringLiteral("例如: GuangNianZhiWai"));
    infoLayout->addRow(QStringLiteral("工程名称:"), m_editName);

    auto *dirLayout = new QHBoxLayout;
    m_labelDir = new QLabel(QStringLiteral("(未选择)"), this);
    m_labelDir->setStyleSheet(QStringLiteral("color: gray;"));
    m_btnBrowseDir = new QPushButton(QStringLiteral("浏览..."), this);
    dirLayout->addWidget(m_labelDir, 1);
    dirLayout->addWidget(m_btnBrowseDir);
    infoLayout->addRow(QStringLiteral("保存位置:"), dirLayout);

    m_comboLanguage = new QComboBox(this);
    m_comboLanguage->addItems({
        QStringLiteral("zh (中文)"),
        QStringLiteral("ja (日本語)"),
        QStringLiteral("en (English)"),
    });
    infoLayout->addRow(QStringLiteral("语言:"), m_comboLanguage);

    m_editSpeaker = new QLineEdit(this);
    m_editSpeaker->setPlaceholderText(QStringLiteral("说话人名称 (可选)"));
    infoLayout->addRow(QStringLiteral("说话人:"), m_editSpeaker);

    // --- Info note ---
    auto *noteLabel = new QLabel(
        QStringLiteral("提示: 音频文件在切片页面中打开和处理，创建工程时无需添加。"), this);
    noteLabel->setWordWrap(true);
    noteLabel->setStyleSheet(QStringLiteral("color: gray; font-style: italic;"));

    // --- Buttons ---
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // --- Main layout ---
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(noteLabel);
    mainLayout->addWidget(m_buttonBox);

    // --- Connections ---
    connect(m_btnBrowseDir, &QPushButton::clicked, this, &NewProjectDialog::onBrowseDir);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &NewProjectDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_editName, &QLineEdit::textChanged, this, [this]() { updateOkButton(); });
}

void NewProjectDialog::onBrowseDir() {
    AppSettings settings(QStringLiteral("DsLabeler"));
    const QString lastDir = settings.get(settings::app::kLastProjectDir);
    dsfw::widgets::FilePathSelector selector(
        {dsfw::widgets::FilePathSelector::Mode::OpenDirectory, QStringLiteral("选择工程保存位置"), {}, {}, lastDir}, this);
    const QString dir = selector.exec();
    if (dir.isEmpty())
        return;
    m_saveDir = dir;
    settings.set(settings::app::kLastProjectDir, dir);
    m_labelDir->setText(QDir::toNativeSeparators(dir));
    m_labelDir->setStyleSheet({});
    updateOkButton();
}

void NewProjectDialog::updateOkButton() {
    bool ok = !m_editName->text().trimmed().isEmpty() && !m_saveDir.isEmpty();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void NewProjectDialog::onAccepted() {
    const QString name = m_editName->text().trimmed();
    const QString projectDir = QDir(m_saveDir).filePath(name);

    // Create project directory
    QDir dir;
    if (!dir.mkpath(projectDir)) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("无法创建目录: %1").arg(projectDir));
        return;
    }

    // Create empty project (no items — slices will be created from slicer page)
    DsProject project;
    project.setWorkingDirectory(projectDir);

    const QString filePath = QDir(projectDir).filePath(name + QStringLiteral(".dsproj"));
    auto saveResult = project.saveFile(filePath);
    if (!saveResult.ok()) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("保存工程失败:\n%1").arg(saveResult.error()));
        return;
    }

    m_projectFilePath = filePath;
    accept();
}

} // namespace dstools
