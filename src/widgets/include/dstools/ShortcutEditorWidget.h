#pragma once

#include <dstools/WidgetsGlobal.h>

#include <QWidget>
#include <QString>

#include <vector>

class QTreeWidget;
class QTreeWidgetItem;
class QKeySequenceEdit;
class QPushButton;

namespace dstools {
class AppSettings;
template <typename T>
struct SettingsKey;
} // namespace dstools

namespace dstools::widgets {

/// Describes a single keyboard shortcut for display and editing.
struct DSTOOLS_WIDGETS_API ShortcutEntry {
    QString displayName;         ///< e.g. "Open directory"
    QString category;            ///< e.g. "File", "Navigation", "Playback", "Tools"
    const char *settingsKeyPath; ///< e.g. "Shortcuts/open" -- the SettingsKey path
    QString defaultSequence;     ///< e.g. "Ctrl+O" -- for reset-to-default
};

/// A reusable widget for viewing and editing keyboard shortcuts.
///
/// Changes are applied immediately to the AppSettings store (which auto-persists
/// and notifies observers). There is no undo/cancel mechanism.
class DSTOOLS_WIDGETS_API ShortcutEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ShortcutEditorWidget(dstools::AppSettings *settings,
                                  const std::vector<ShortcutEntry> &entries,
                                  QWidget *parent = nullptr);
    ~ShortcutEditorWidget() override;

    /// Show a modal dialog wrapping this widget.
    /// Note: changes are applied immediately via settings; Cancel does not undo them.
    static void showDialog(dstools::AppSettings *settings,
                           const std::vector<ShortcutEntry> &entries,
                           QWidget *parent);

private:
    void buildTree();
    void resetEntry(int entryIndex);
    void resetAll();
    void applyShortcut(int entryIndex, const QString &newSequence);
    QString currentSequence(int entryIndex) const;
    int findConflict(int excludeIndex, const QString &sequence) const;

    dstools::AppSettings *m_settings;
    std::vector<ShortcutEntry> m_entries;
    QTreeWidget *m_tree;
    QPushButton *m_resetAllBtn;

    /// Maps entry index -> the tree widget item displaying it.
    std::vector<QTreeWidgetItem *> m_entryItems;
};

} // namespace dstools::widgets
