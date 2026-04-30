#include <dstools/ShortcutEditorWidget.h>
#include <dsfw/AppSettings.h>
#include <dstools/FramelessHelper.h>

#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <map>

namespace dstools::widgets {

// Column indices
enum Column {
    ColAction = 0,
    ColShortcut = 1,
    ColReset = 2,
};

// Role to store entry index on child items
static constexpr int EntryIndexRole = Qt::UserRole + 1;

ShortcutEditorWidget::ShortcutEditorWidget(dstools::AppSettings *settings,
                                           const std::vector<ShortcutEntry> &entries,
                                           QWidget *parent)
    : QWidget(parent), m_settings(settings), m_entries(entries) {
    auto *layout = new QVBoxLayout(this);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({tr("Action"), tr("Shortcut"), QString()});
    m_tree->setAlternatingRowColors(true);
    m_tree->setRootIsDecorated(true);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(m_tree);

    m_resetAllBtn = new QPushButton(tr("Reset All"), this);
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    bottomLayout->addWidget(m_resetAllBtn);
    layout->addLayout(bottomLayout);

    connect(m_resetAllBtn, &QPushButton::clicked, this, &ShortcutEditorWidget::resetAll);

    buildTree();

    // Column sizing
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(ColAction, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(ColShortcut, QHeaderView::Interactive);
    m_tree->header()->setSectionResizeMode(ColReset, QHeaderView::Fixed);
    m_tree->setColumnWidth(ColShortcut, 150);
    m_tree->setColumnWidth(ColReset, 60);

    // Double-click to edit shortcut
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem *item, int column) {
                if (column != ColShortcut)
                    return;
                QVariant v = item->data(0, EntryIndexRole);
                if (!v.isValid())
                    return; // category node

                // Commit any previously active editor first
                commitPendingEdit();

                int idx = v.toInt();

                auto *editor = new QKeySequenceEdit(
                    QKeySequence(currentSequence(idx)), m_tree);
                m_tree->setItemWidget(item, ColShortcut, editor);
                editor->setFocus();

                m_activeEditor = editor;
                m_activeItem = item;
                m_activeEntryIndex = idx;

                connect(editor, &QKeySequenceEdit::editingFinished, this,
                        [this]() {
                            commitPendingEdit();
                        });
            });
}

void ShortcutEditorWidget::commitPendingEdit() {
    if (!m_activeEditor)
        return;

    QString seq = m_activeEditor->keySequence().toString();
    int idx = m_activeEntryIndex;
    QTreeWidgetItem *item = m_activeItem;

    // Clear active state before modifying tree (removeItemWidget destroys the editor)
    m_activeEditor = nullptr;
    m_activeItem = nullptr;
    m_activeEntryIndex = -1;

    m_tree->removeItemWidget(item, ColShortcut);
    if (!seq.isEmpty()) {
        applyShortcut(idx, seq);
    }
    item->setText(ColShortcut, currentSequence(idx));
}

ShortcutEditorWidget::~ShortcutEditorWidget() {
    commitPendingEdit();
}

static constexpr int kDialogWidth = 520;
static constexpr int kDialogHeight = 420;

void ShortcutEditorWidget::showDialog(dstools::AppSettings *settings,
                                      const std::vector<ShortcutEntry> &entries,
                                      QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle(QObject::tr("Keyboard Shortcuts"));
    dlg.resize(kDialogWidth, kDialogHeight);

    dstools::FramelessHelper::applyToDialog(&dlg);

    // applyToDialog creates a QVBoxLayout with title bar; add content below
    auto *layout = qobject_cast<QVBoxLayout *>(dlg.layout());
    auto *widget = new ShortcutEditorWidget(settings, entries, &dlg);
    layout->addWidget(widget);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted, widget, &ShortcutEditorWidget::commitPendingEdit);
    QObject::connect(buttons, &QDialogButtonBox::rejected, widget, &ShortcutEditorWidget::commitPendingEdit);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}

void ShortcutEditorWidget::buildTree() {
    m_tree->clear();
    m_entryItems.clear();
    m_entryItems.resize(m_entries.size(), nullptr);

    // Group entries by category, preserving order of first appearance.
    std::map<QString, QTreeWidgetItem *> categoryNodes;

    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        const auto &entry = m_entries[i];

        // Get or create category node
        QTreeWidgetItem *catItem = nullptr;
        auto it = categoryNodes.find(entry.category);
        if (it != categoryNodes.end()) {
            catItem = it->second;
        } else {
            catItem = new QTreeWidgetItem(m_tree);
            catItem->setText(ColAction, entry.category);
            catItem->setFlags(Qt::ItemIsEnabled);
            catItem->setExpanded(true);
            auto font = catItem->font(ColAction);
            font.setBold(true);
            catItem->setFont(ColAction, font);
            categoryNodes[entry.category] = catItem;
        }

        // Create child item
        auto *child = new QTreeWidgetItem(catItem);
        child->setText(ColAction, entry.displayName);
        child->setText(ColShortcut, currentSequence(i));
        child->setData(0, EntryIndexRole, i);
        child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_entryItems[i] = child;

        // Reset button
        auto *resetBtn = new QPushButton(tr("Reset"), m_tree);
        resetBtn->setFixedWidth(54);
        m_tree->setItemWidget(child, ColReset, resetBtn);
        connect(resetBtn, &QPushButton::clicked, this, [this, i]() { resetEntry(i); });
    }

    m_tree->expandAll();
}

QString ShortcutEditorWidget::currentSequence(int entryIndex) const {
    const auto &entry = m_entries[entryIndex];
    dstools::SettingsKey<QString> key(entry.settingsKeyPath, entry.defaultSequence);
    return m_settings->get(key);
}

int ShortcutEditorWidget::findConflict(int excludeIndex, const QString &sequence) const {
    if (sequence.isEmpty())
        return -1;
    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        if (i == excludeIndex)
            continue;
        if (currentSequence(i).compare(sequence, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

void ShortcutEditorWidget::applyShortcut(int entryIndex, const QString &newSequence) {
    int conflict = findConflict(entryIndex, newSequence);
    if (conflict >= 0) {
        const auto &other = m_entries[conflict];
        auto result = QMessageBox::warning(
            this, tr("Shortcut Conflict"),
            tr("The shortcut \"%1\" is already assigned to \"%2\" (%3).\n"
               "Do you want to reassign it?")
                .arg(newSequence, other.displayName, other.category),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result != QMessageBox::Yes)
            return;
        // Clear the conflicting entry
        const auto &conflictEntry = m_entries[conflict];
        dstools::SettingsKey<QString> conflictKey(conflictEntry.settingsKeyPath,
                                                  conflictEntry.defaultSequence);
        m_settings->set(conflictKey, QString());
        if (m_entryItems[conflict])
            m_entryItems[conflict]->setText(ColShortcut, QString());
    }

    const auto &entry = m_entries[entryIndex];
    dstools::SettingsKey<QString> key(entry.settingsKeyPath, entry.defaultSequence);
    m_settings->set(key, newSequence);
    if (m_entryItems[entryIndex])
        m_entryItems[entryIndex]->setText(ColShortcut, newSequence);
}

void ShortcutEditorWidget::resetEntry(int entryIndex) {
    const auto &entry = m_entries[entryIndex];
    applyShortcut(entryIndex, entry.defaultSequence);
}

void ShortcutEditorWidget::resetAll() {
    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        const auto &entry = m_entries[i];
        dstools::SettingsKey<QString> key(entry.settingsKeyPath, entry.defaultSequence);
        m_settings->set(key, entry.defaultSequence);
        if (m_entryItems[i])
            m_entryItems[i]->setText(ColShortcut, entry.defaultSequence);
    }
}

} // namespace dstools::widgets
