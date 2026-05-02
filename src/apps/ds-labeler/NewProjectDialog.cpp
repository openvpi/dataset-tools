#include "NewProjectDialog.h"

#include <dstools/DsProject.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
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

    // --- Audio files ---
    auto *audioGroup = new QGroupBox(QStringLiteral("音频文件"), this);
    auto *audioLayout = new QVBoxLayout(audioGroup);

    m_audioList = new QListWidget(this);
    m_audioList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    audioLayout->addWidget(m_audioList);

    auto *audioBtnLayout = new QHBoxLayout;
    m_btnAddAudio = new QPushButton(QStringLiteral("添加..."), this);
    m_btnRemoveAudio = new QPushButton(QStringLiteral("移除"), this);
    audioBtnLayout->addWidget(m_btnAddAudio);
    audioBtnLayout->addWidget(m_btnRemoveAudio);
    audioBtnLayout->addStretch();
    audioLayout->addLayout(audioBtnLayout);

    // --- Buttons ---
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("创建"));
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // --- Main layout ---
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(infoGroup);
    mainLayout->addWidget(audioGroup);
    mainLayout->addWidget(m_buttonBox);

    // --- Connections ---
    connect(m_btnBrowseDir, &QPushButton::clicked, this, &NewProjectDialog::onBrowseDir);
    connect(m_btnAddAudio, &QPushButton::clicked, this, &NewProjectDialog::onAddAudio);
    connect(m_btnRemoveAudio, &QPushButton::clicked, this, &NewProjectDialog::onRemoveAudio);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &NewProjectDialog::onAccepted);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_editName, &QLineEdit::textChanged, this, [this]() { updateOkButton(); });
}

void NewProjectDialog::onBrowseDir() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择工程保存位置"));
    if (dir.isEmpty())
        return;
    m_saveDir = dir;
    m_labelDir->setText(QDir::toNativeSeparators(dir));
    m_labelDir->setStyleSheet({});
    updateOkButton();
}

void NewProjectDialog::onAddAudio() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择音频文件"), {},
        QStringLiteral("音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg);;所有文件 (*)"));
    for (const auto &f : files) {
        // Avoid duplicates
        bool found = false;
        for (int i = 0; i < m_audioList->count(); ++i) {
            if (m_audioList->item(i)->data(Qt::UserRole).toString() == f) {
                found = true;
                break;
            }
        }
        if (!found) {
            auto *item = new QListWidgetItem(QFileInfo(f).fileName());
            item->setData(Qt::UserRole, f);
            m_audioList->addItem(item);
        }
    }
    updateOkButton();
}

void NewProjectDialog::onRemoveAudio() {
    qDeleteAll(m_audioList->selectedItems());
    updateOkButton();
}

void NewProjectDialog::updateOkButton() {
    bool ok = !m_editName->text().trimmed().isEmpty() && !m_saveDir.isEmpty() &&
              m_audioList->count() > 0;
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void NewProjectDialog::onAccepted() {
    const QString name = m_editName->text().trimmed();
    const QString projectDir = m_saveDir + QStringLiteral("/") + name;

    // Create project directory
    QDir dir;
    if (!dir.mkpath(projectDir)) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("无法创建目录: %1").arg(projectDir));
        return;
    }

    // Parse language code from combo text
    const QString langText = m_comboLanguage->currentText();
    const QString langCode = langText.section(' ', 0, 0); // "zh", "ja", "en"

    // Build items from audio files
    std::vector<Item> items;
    for (int i = 0; i < m_audioList->count(); ++i) {
        const QString audioPath = m_audioList->item(i)->data(Qt::UserRole).toString();
        const QFileInfo fi(audioPath);

        Item item;
        item.id = fi.completeBaseName();
        item.name = fi.completeBaseName();
        item.language = langCode;
        item.speaker = m_editSpeaker->text().trimmed();
        item.audioSource = DsProject::toPosixPath(audioPath);
        items.push_back(std::move(item));
    }

    // Create and save project
    DsProject project;
    project.setWorkingDirectory(projectDir);
    project.setItems(std::move(items));

    const QString filePath = projectDir + QStringLiteral("/") + name +
                             QStringLiteral(".dsproj");
    QString error;
    if (!project.save(filePath, error)) {
        QMessageBox::warning(this, QStringLiteral("错误"),
                             QStringLiteral("保存工程失败:\n%1").arg(error));
        return;
    }

    m_projectFilePath = filePath;
    accept();
}

} // namespace dstools
