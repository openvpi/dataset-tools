#include <dsfw/widgets/DroppableFileListPanel.h>
#include <dsfw/widgets/FileProgressTracker.h>

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

namespace dsfw::widgets {

DroppableFileListPanel::DroppableFileListPanel(QWidget *parent) : QWidget(parent) {
    setAcceptDrops(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Top button bar ────────────────────────────────────────────────────
    auto *btnLayout = new QHBoxLayout;
    btnLayout->setContentsMargins(2, 2, 2, 2);
    btnLayout->setSpacing(2);
    m_btnAddDir = new QPushButton(QStringLiteral("📁"), this);
    m_btnAdd = new QPushButton(QStringLiteral("+"), this);
    m_btnRemove = new QPushButton(QStringLiteral("−"), this);
    m_btnDiscard = new QPushButton(QStringLiteral("Discard"), this);
    m_btnClear = new QPushButton(QStringLiteral("Clear"), this);
    m_btnAddDir->setToolTip(tr("Add directory"));
    m_btnAdd->setToolTip(tr("Add files"));
    m_btnRemove->setToolTip(tr("Remove selected"));
    m_btnDiscard->setToolTip(tr("Discard selected (mark as skipped)"));
    m_btnClear->setToolTip(tr("Clear all files"));
    m_btnAddDir->setFixedWidth(32);
    m_btnAdd->setFixedWidth(32);
    m_btnRemove->setFixedWidth(32);
    btnLayout->addWidget(m_btnAddDir);
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnRemove);
    btnLayout->addWidget(m_btnDiscard);
    btnLayout->addWidget(m_btnClear);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listWidget->setAlternatingRowColors(true);
    layout->addWidget(m_listWidget, 1);

    m_progressTracker = new FileProgressTracker(FileProgressTracker::LabelOnly, this);
    m_progressTracker->setVisible(false);
    layout->addWidget(m_progressTracker);

    connect(m_btnAdd, &QPushButton::clicked, this, &DroppableFileListPanel::onAddFiles);
    connect(m_btnAddDir, &QPushButton::clicked, this, &DroppableFileListPanel::onAddDirectory);
    connect(m_btnRemove, &QPushButton::clicked, this, &DroppableFileListPanel::onRemoveSelected);
    connect(m_btnDiscard, &QPushButton::clicked, this, &DroppableFileListPanel::onDiscardSelected);
    connect(m_btnClear, &QPushButton::clicked, this, &DroppableFileListPanel::onClearAll);
    connect(m_listWidget, &QListWidget::currentRowChanged, this,
            &DroppableFileListPanel::onCurrentRowChanged);
}

DroppableFileListPanel::~DroppableFileListPanel() = default;

void DroppableFileListPanel::setFileFilters(const QStringList &filters) {
    m_filters = filters;
}

QStringList DroppableFileListPanel::fileFilters() const {
    return m_filters;
}

bool DroppableFileListPanel::matchesFilter(const QString &filePath) const {
    if (m_filters.isEmpty())
        return true;

    const QString fileName = QFileInfo(filePath).fileName();
    for (const auto &filter : m_filters) {
        // Convert glob pattern to regex: *.wav → .*\.wav
        auto re = QRegularExpression::fromWildcard(filter, Qt::CaseInsensitive);
        if (re.match(fileName).hasMatch())
            return true;
    }
    return false;
}

void DroppableFileListPanel::addFiles(const QStringList &paths) {
    QStringList added;
    for (const auto &path : paths) {
        if (!matchesFilter(path))
            continue;

        // Deduplicate
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
            styleItem(item, path);
            m_listWidget->addItem(item);
            added.append(path);
        }
    }
    if (!added.isEmpty())
        emit filesAdded(added);
}

void DroppableFileListPanel::addDirectory(const QString &dirPath) {
    QDir dir(dirPath);
    if (!dir.exists())
        return;

    QStringList files;
    const auto entries = dir.entryInfoList(m_filters, QDir::Files | QDir::Readable, QDir::Name);
    for (const auto &entry : entries)
        files.append(entry.absoluteFilePath());

    if (!files.isEmpty())
        addFiles(files);
}

void DroppableFileListPanel::clear() {
    m_listWidget->clear();
}

QStringList DroppableFileListPanel::filePaths() const {
    QStringList paths;
    for (int i = 0; i < m_listWidget->count(); ++i)
        paths.append(m_listWidget->item(i)->data(Qt::UserRole).toString());
    return paths;
}

QString DroppableFileListPanel::currentFilePath() const {
    auto *item = m_listWidget->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void DroppableFileListPanel::setCurrentFile(const QString &path) {
    const QFileInfo fi(path);
    for (int i = 0; i < m_listWidget->count(); ++i) {
        if (m_listWidget->item(i)->data(Qt::UserRole).toString() == path ||
            QFileInfo(m_listWidget->item(i)->data(Qt::UserRole).toString()).fileName() == fi.fileName()) {
            m_listWidget->setCurrentRow(i);
            return;
        }
    }
}

void DroppableFileListPanel::selectNext() {
    int row = m_listWidget->currentRow();
    if (row + 1 < m_listWidget->count())
        m_listWidget->setCurrentRow(row + 1);
}

void DroppableFileListPanel::selectPrev() {
    int row = m_listWidget->currentRow();
    if (row > 0)
        m_listWidget->setCurrentRow(row - 1);
}

int DroppableFileListPanel::fileCount() const {
    return m_listWidget->count();
}

void DroppableFileListPanel::setShowProgress(bool show) {
    if (m_progressTracker)
        m_progressTracker->setVisible(show);
}

FileProgressTracker *DroppableFileListPanel::progressTracker() const {
    return m_progressTracker;
}

QListWidget *DroppableFileListPanel::listWidget() const {
    return m_listWidget;
}

void DroppableFileListPanel::styleItem(QListWidgetItem * /*item*/, const QString &filePath) {
    Q_UNUSED(filePath)
    // Default: filename only. Subclasses can override for custom styling.
}

void DroppableFileListPanel::onAddFiles() {
    // Build filter string from m_filters: "*.wav *.flac" → "Audio Files (*.wav *.flac)"
    QString filterStr;
    if (!m_filters.isEmpty()) {
        filterStr = tr("Matched Files") + QStringLiteral(" (") + m_filters.join(' ') + QStringLiteral(")");
        filterStr += QStringLiteral(";;") + tr("All Files") + QStringLiteral(" (*)");
    }

    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Add Files"), {}, filterStr);
    if (!files.isEmpty())
        addFiles(files);
}

void DroppableFileListPanel::onAddDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Add Directory"));
    if (!dir.isEmpty())
        addDirectory(dir);
}

void DroppableFileListPanel::onRemoveSelected() {
    qDeleteAll(m_listWidget->selectedItems());
    emit filesRemoved();
}

void DroppableFileListPanel::onDiscardSelected() {
    const auto selected = m_listWidget->selectedItems();
    for (auto *item : selected) {
        item->setForeground(Qt::gray);
        QFont f = item->font();
        f.setStrikeOut(true);
        item->setFont(f);
        item->setData(Qt::UserRole + 1, true); // discarded flag
    }
}

void DroppableFileListPanel::onClearAll() {
    m_listWidget->clear();
    emit filesRemoved();
}

void DroppableFileListPanel::onCurrentRowChanged(int row) {
    if (row < 0 || row >= m_listWidget->count())
        return;
    emit fileSelected(m_listWidget->item(row)->data(Qt::UserRole).toString());
}

void DroppableFileListPanel::dragEnterEvent(QDragEnterEvent *event) {
    if (!event->mimeData()->hasUrls())
        return;

    // Accept if at least one URL matches our filter or is a directory
    for (const auto &url : event->mimeData()->urls()) {
        const QString path = url.toLocalFile();
        if (path.isEmpty())
            continue;
        QFileInfo fi(path);
        if (fi.isDir() || matchesFilter(path)) {
            event->acceptProposedAction();
            return;
        }
    }
}

void DroppableFileListPanel::dragMoveEvent(QDragMoveEvent *event) {
    event->acceptProposedAction();
}

void DroppableFileListPanel::dropEvent(QDropEvent *event) {
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

} // namespace dsfw::widgets
