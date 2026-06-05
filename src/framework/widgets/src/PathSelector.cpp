#include <dsfw/widgets/PathSelector.h>

#include <dsfw/FileDialogHelper.h>
#include <dsfw/widgets/RecentPathStore.h>

#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>

namespace dsfw::widgets {

/// Check if a filename matches a Qt file dialog filter string.
/// e.g. filter = "Audio (*.wav *.mp3);;All (*)" → matches "test.wav"
static bool matchesDialogFilter(const QString &fileName, const QString &filter) {
    if (filter.isEmpty())
        return true;

    // Parse glob patterns from all filter groups: "Name (*.ext1 *.ext2);;Name2 (*.ext3)"
    static const QRegularExpression re(R"(\*\.\w+)");
    auto it = re.globalMatch(filter);
    while (it.hasNext()) {
        auto match = it.next();
        auto glob = QRegularExpression::fromWildcard(match.captured(), Qt::CaseInsensitive);
        if (glob.match(fileName).hasMatch())
            return true;
    }
    // If filter contains (*), accept all
    return filter.contains(QStringLiteral("(*)"));
}

PathSelector::PathSelector(Mode mode, const QString &label,
                           const QString &filter, QWidget *parent,
                           const QString &settingsKey)
    : QWidget(parent), m_mode(mode), m_filter(filter), m_settingsKey(settingsKey) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_label = new QLabel(label, this);
    m_lineEdit = new QLineEdit(this);
    m_browseBtn = new QPushButton(QStringLiteral("..."), this);
    m_browseBtn->setFixedWidth(30);

    layout->addWidget(m_label);
    layout->addWidget(m_lineEdit, 1);
    layout->addWidget(m_browseBtn);

    connect(m_browseBtn, &QPushButton::clicked, this, &PathSelector::onBrowseClicked);
    connect(m_lineEdit, &QLineEdit::textChanged, this, &PathSelector::pathChanged);

    setAcceptDrops(true);
}

QString PathSelector::path() const {
    return m_lineEdit->text();
}

void PathSelector::setPath(const QString &path) {
    if (m_lineEdit->text() != path)
        m_lineEdit->setText(path);
}

void PathSelector::setPlaceholder(const QString &text) {
    m_lineEdit->setPlaceholderText(text);
}

void PathSelector::setDefaultSuffix(const QString &suffix) {
    m_defaultSuffix = suffix;
}

void PathSelector::setDialogTitle(const QString &title) {
    m_dialogTitle = title;
}

PathSelector::Mode PathSelector::mode() const {
    return m_mode;
}

QLineEdit *PathSelector::lineEdit() const {
    return m_lineEdit;
}

void PathSelector::dragEnterEvent(QDragEnterEvent *event) {
    if (!event->mimeData()->hasUrls()) return;

    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    const QString filePath = urls.first().toLocalFile();
    if (filePath.isEmpty()) return;

    const QFileInfo info(filePath);
    switch (m_mode) {
    case Directory:
        if (info.isDir())
            event->acceptProposedAction();
        break;
    case OpenFile:
    case SaveFile:
        if (info.isFile() && matchesDialogFilter(info.fileName(), m_filter))
            event->acceptProposedAction();
        break;
    }
}

void PathSelector::dropEvent(QDropEvent *event) {
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    const QString filePath = urls.first().toLocalFile();
    if (filePath.isEmpty()) return;

    setPath(filePath);
}

void PathSelector::onBrowseClicked() {
    dsfw::FileDialogHelper::Options opts;
    opts.parent = this;
    opts.title = m_dialogTitle.isEmpty() ? m_label->text() : m_dialogTitle;
    opts.nameFilters = {m_filter};
    opts.defaultSuffix = m_defaultSuffix;

    QString defaultDir;
    if (!path().isEmpty()) {
        defaultDir = QFileInfo(path()).absolutePath();
    } else {
        defaultDir = RecentPathStore::load(m_settingsKey);
    }
    opts.defaultDir = defaultDir;

    QString result;
    switch (m_mode) {
    case OpenFile:
        result = dsfw::FileDialogHelper::getOpenFileName(opts);
        break;
    case SaveFile:
        result = dsfw::FileDialogHelper::getSaveFileName(opts);
        break;
    case Directory:
        opts.defaultDir = path().isEmpty() ? defaultDir : path();
        result = dsfw::FileDialogHelper::getExistingDirectory(opts);
        break;
    }
    if (!result.isEmpty()) {
        setPath(result);
        saveRecentDir(result);
    }
}

void PathSelector::saveRecentDir(const QString &path) {
    RecentPathStore::save(m_settingsKey, path);
}

} // namespace dsfw::widgets
