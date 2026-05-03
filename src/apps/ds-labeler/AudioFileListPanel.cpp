#include "AudioFileListPanel.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMimeData>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace dstools {

static const QStringList kAudioExtensions = {
    QStringLiteral("wav"), QStringLiteral("mp3"), QStringLiteral("flac"),
    QStringLiteral("m4a"), QStringLiteral("ogg"), QStringLiteral("opus"),
    QStringLiteral("wma"), QStringLiteral("aac"),
};

static bool isAudioFile(const QString &path) {
    const QString ext = QFileInfo(path).suffix().toLower();
    return kAudioExtensions.contains(ext);
}

AudioFileListPanel::AudioFileListPanel(QWidget *parent) : QWidget(parent) {
    setAcceptDrops(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listWidget->setAlternatingRowColors(true);
    layout->addWidget(m_listWidget);

    auto *btnLayout = new QHBoxLayout;
    btnLayout->setContentsMargins(2, 0, 2, 2);
    m_btnAdd = new QPushButton(QStringLiteral("+"), this);
    m_btnAddDir = new QPushButton(QStringLiteral("📁"), this);
    m_btnRemove = new QPushButton(QStringLiteral("−"), this);
    m_btnAdd->setToolTip(QStringLiteral("添加音频文件"));
    m_btnAddDir->setToolTip(QStringLiteral("添加音频目录"));
    m_btnRemove->setToolTip(QStringLiteral("移除选中"));
    m_btnAdd->setFixedWidth(32);
    m_btnAddDir->setFixedWidth(32);
    m_btnRemove->setFixedWidth(32);
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnAddDir);
    btnLayout->addWidget(m_btnRemove);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(m_btnAdd, &QPushButton::clicked, this, &AudioFileListPanel::onAddFiles);
    connect(m_btnAddDir, &QPushButton::clicked, this, &AudioFileListPanel::onAddDirectory);
    connect(m_btnRemove, &QPushButton::clicked, this, &AudioFileListPanel::onRemoveSelected);
    connect(m_listWidget, &QListWidget::currentRowChanged, this,
            &AudioFileListPanel::onCurrentRowChanged);
}

void AudioFileListPanel::addFiles(const QStringList &paths) {
    QStringList added;
    for (const auto &path : paths) {
        if (!isAudioFile(path))
            continue;

        // Check for duplicates
        bool found = false;
        for (int i = 0; i < m_listWidget->count(); ++i) {
            if (m_listWidget->item(i)->data(Qt::UserRole).toString() == path) {
                found = true;
                break;
            }
        }
        if (!found) {
            auto *item = new QListWidgetItem(QFileInfo(path).fileName());
            item->setData(Qt::UserRole, path);
            m_listWidget->addItem(item);
            added.append(path);
        }
    }
    if (!added.isEmpty())
        emit filesAdded(added);
}

void AudioFileListPanel::addDirectory(const QString &dirPath) {
    QDir dir(dirPath);
    QStringList files;
    const auto entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        if (isAudioFile(entry.absoluteFilePath()))
            files.append(entry.absoluteFilePath());
    }
    if (!files.isEmpty())
        addFiles(files);
}

QStringList AudioFileListPanel::filePaths() const {
    QStringList paths;
    for (int i = 0; i < m_listWidget->count(); ++i)
        paths.append(m_listWidget->item(i)->data(Qt::UserRole).toString());
    return paths;
}

QString AudioFileListPanel::currentFilePath() const {
    auto *item = m_listWidget->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

int AudioFileListPanel::fileCount() const {
    return m_listWidget->count();
}

void AudioFileListPanel::onAddFiles() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("选择音频文件"), {},
        QStringLiteral("音频文件 (*.wav *.mp3 *.flac *.m4a *.ogg *.opus);;所有文件 (*)"));
    if (!files.isEmpty())
        addFiles(files);
}

void AudioFileListPanel::onAddDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择音频目录"));
    if (!dir.isEmpty())
        addDirectory(dir);
}

void AudioFileListPanel::onRemoveSelected() {
    qDeleteAll(m_listWidget->selectedItems());
}

void AudioFileListPanel::onCurrentRowChanged(int row) {
    if (row < 0 || row >= m_listWidget->count())
        return;
    emit fileSelected(m_listWidget->item(row)->data(Qt::UserRole).toString());
}

void AudioFileListPanel::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void AudioFileListPanel::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    QStringList files;
    QStringList dirs;
    for (const auto &url : urls) {
        const QString path = url.toLocalFile();
        if (path.isEmpty())
            continue;
        QFileInfo fi(path);
        if (fi.isDir())
            dirs.append(path);
        else if (fi.isFile())
            files.append(path);
    }

    if (!files.isEmpty())
        addFiles(files);
    for (const auto &dir : dirs)
        addDirectory(dir);

    event->acceptProposedAction();
}

} // namespace dstools
