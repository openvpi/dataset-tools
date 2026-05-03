#include <dsfw/widgets/PathSelector.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
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
                           const QString &filter, QWidget *parent)
    : QWidget(parent), m_mode(mode), m_filter(filter) {
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
    QString result;
    const QString startDir = path().isEmpty() ? QString() : QFileInfo(path()).absolutePath();
    switch (m_mode) {
    case OpenFile:
        result = QFileDialog::getOpenFileName(this, m_label->text(), startDir, m_filter);
        break;
    case SaveFile:
        result = QFileDialog::getSaveFileName(this, m_label->text(), startDir, m_filter);
        break;
    case Directory:
        result = QFileDialog::getExistingDirectory(this, m_label->text(),
                                                    path().isEmpty() ? QString() : path());
        break;
    }
    if (!result.isEmpty())
        setPath(result);
}

} // namespace dsfw::widgets
