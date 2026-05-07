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
#include <QRegularExpression>
#include <QStyle>
#include <QToolButton>
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
    m_btnAddDir = new QToolButton(this);
    m_btnAddDir->setIcon(style()->standardIcon(QStyle::SP_DirOpenIcon));
    m_btnAddDir->setToolTip(tr("Add directory"));
    m_btnAddDir->setAutoRaise(true);

    m_btnAdd = new QToolButton(this);
    m_btnAdd->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    m_btnAdd->setToolTip(tr("Add files"));
    m_btnAdd->setAutoRaise(true);

    m_btnRemove = new QToolButton(this);
    m_btnRemove->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_btnRemove->setToolTip(tr("Remove selected"));
    m_btnRemove->setAutoRaise(true);

    m_btnDiscard = new QToolButton(this);
    m_btnDiscard->setText(tr("Discard"));
    m_btnDiscard->setToolTip(tr("Discard selected (mark as skipped)"));
    m_btnDiscard->setAutoRaise(true);

    m_btnClear = new QToolButton(this);
    m_btnClear->setText(tr("Clear"));
    m_btnClear->setToolTip(tr("Clear all files"));
    m_btnClear->setAutoRaise(true);
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

    connect(m_btnAdd, &QToolButton::clicked, this, &DroppableFileListPanel::onAddFiles);
    connect(m_btnAddDir, &QToolButton::clicked, this, &DroppableFileListPanel::onAddDirectory);
    connect(m_btnRemove, &QToolButton::clicked, this, &DroppableFileListPanel::onRemoveSelected);
    connect(m_btnDiscard, &QToolButton::clicked, this, &DroppableFileListPanel::onDiscardSelected);
    connect(m_btnClear, &QToolButton::clicked, this, &DroppableFileListPanel::onClearAll);
    connect(m_listWidget, &QListWidget::currentRowChanged, this,
            &DroppableFileListPanel::onCurrentRowChanged);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this,
            [this]() { updateDiscardButtonText(); });
}

DroppableFileListPanel::~DroppableFileListPanel() = default;

void DroppableFileListPanel::setButtonVisible(const QString &name, bool visible) {
    QToolButton *btn = nullptr;
    if (name == QStringLiteral("addDir"))
        btn = m_btnAddDir;
    else if (name == QStringLiteral("add"))
        btn = m_btnAdd;
    else if (name == QStringLiteral("remove"))
        btn = m_btnRemove;
    else if (name == QStringLiteral("discard"))
        btn = m_btnDiscard;
    else if (name == QStringLiteral("clear"))
        btn = m_btnClear;
    if (btn)
        btn->setVisible(visible);
}

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
        bool discarded = item->data(Qt::UserRole + 1).toBool();
        if (discarded) {
            item->setData(Qt::UserRole + 1, false);
            item->setForeground(Qt::black);
            QFont f = item->font();
            f.setStrikeOut(false);
            item->setFont(f);
        } else {
            item->setData(Qt::UserRole + 1, true);
            item->setForeground(Qt::gray);
            QFont f = item->font();
            f.setStrikeOut(true);
            item->setFont(f);
        }
    }
    updateDiscardButtonText();
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

void DroppableFileListPanel::updateDiscardButtonText() {
    const auto selected = m_listWidget->selectedItems();
    bool anyDiscarded = false;
    for (auto *item : selected) {
        if (item->data(Qt::UserRole + 1).toBool()) {
            anyDiscarded = true;
            break;
        }
    }
    m_btnDiscard->setText(anyDiscarded ? tr("Restore") : tr("Discard"));
    m_btnDiscard->setToolTip(anyDiscarded ? tr("Restore selected (unmark as skipped)")
                                          : tr("Discard selected (mark as skipped)"));
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
