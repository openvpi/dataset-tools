#include "EditorPageBase.h"
#include "SliceListPanel.h"
#include "ISettingsBackend.h"

#include <dsfw/CommonKeys.h>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>

namespace dstools {

static const dstools::SettingsKey<QString> kSplitterState("Layout/splitterState", "");

EditorPageBase::EditorPageBase(const QString &settingsGroup, QWidget *parent)
    : QWidget(parent), m_settings(settingsGroup) {
    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
}

EditorPageBase::~EditorPageBase() = default;

// ── Setup helpers ─────────────────────────────────────────────────────────────

void EditorPageBase::setupBaseLayout(QWidget *editorWidget) {
    m_sliceList = new SliceListPanel(this);
    m_sliceList->setMinimumWidth(160);
    m_sliceList->setMaximumWidth(280);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_sliceList);
    m_splitter->addWidget(editorWidget);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_splitter);

    connect(m_sliceList, &SliceListPanel::sliceSelected,
            this, &EditorPageBase::onSliceSelected);
}

void EditorPageBase::setupNavigationActions() {
    m_prevAction = new QAction(QStringLiteral("上一个切片"), this);
    m_nextAction = new QAction(QStringLiteral("下一个切片"), this);

    m_shortcutManager->bind(m_prevAction, dsfw::CommonKeys::NavigationPrev,
                            QStringLiteral("上一个切片"), QStringLiteral("导航"));
    m_shortcutManager->bind(m_nextAction, dsfw::CommonKeys::NavigationNext,
                            QStringLiteral("下一个切片"), QStringLiteral("导航"));

    connect(m_prevAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectPrev();
    });
    connect(m_nextAction, &QAction::triggered, this, [this]() {
        m_sliceList->selectNext();
    });
}

// ── Data source ───────────────────────────────────────────────────────────────

void EditorPageBase::setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend) {
    m_source = source;
    m_settingsBackend = settingsBackend;
    if (m_sliceList)
        m_sliceList->setDataSource(source);
}

// ── Slice selection ───────────────────────────────────────────────────────────

void EditorPageBase::onSliceSelected(const QString &sliceId) {
    if (!maybeSave())
        return;

    m_currentSliceId = sliceId;
    if (m_sliceList)
        m_sliceList->saveSelection(m_settings);

    if (!m_source)
        return;

    onSliceSelectedImpl(sliceId);
    emit sliceChanged(sliceId);
}

// ── IPageActions ──────────────────────────────────────────────────────────────

QString EditorPageBase::windowTitle() const {
    QString title = windowTitlePrefix();
    if (!m_currentSliceId.isEmpty())
        title += QStringLiteral(" — ") + m_currentSliceId;
    return title;
}

bool EditorPageBase::hasUnsavedChanges() const {
    return isDirty();
}

void EditorPageBase::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void EditorPageBase::handleDrop(QDropEvent *event) {
    Q_UNUSED(event)
}

// ── IPageLifecycle ────────────────────────────────────────────────────────────

void EditorPageBase::onActivated() {
    if (m_shortcutManager)
        m_shortcutManager->setEnabled(true);

    if (m_sliceList)
        m_sliceList->refresh();

    // Restore splitter state
    auto state = m_settings.get(kSplitterState);
    if (!state.isEmpty() && m_splitter)
        m_splitter->restoreState(QByteArray::fromBase64(state.toUtf8()));

    restoreExtraSplitters();

    if (m_currentSliceId.isEmpty() && m_sliceList)
        m_sliceList->ensureSelection(m_settings);

    onAutoInfer();
}

bool EditorPageBase::onDeactivating() {
    // Save splitter state
    if (m_splitter)
        m_settings.set(kSplitterState, QString::fromLatin1(m_splitter->saveState().toBase64()));

    saveExtraSplitters();

    return maybeSave();
}

void EditorPageBase::onDeactivated() {
    if (m_shortcutManager)
        m_shortcutManager->setEnabled(false);

    onDeactivatedImpl();
}

void EditorPageBase::onShutdown() {
    if (m_shortcutManager)
        m_shortcutManager->saveAll();

    onShutdownImpl();
}

// ── Utility ───────────────────────────────────────────────────────────────────

bool EditorPageBase::maybeSave() {
    if (!isDirty())
        return true;

    auto ret = QMessageBox::question(
        this, QStringLiteral("未保存的更改"),
        QStringLiteral("当前切片已修改，是否保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save)
        return saveCurrentSlice();
    if (ret == QMessageBox::Discard)
        return true;
    return false; // Cancel
}

} // namespace dstools
