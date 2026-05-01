#pragma once
/// @file ShortcutEditorWidget.h
/// @brief Tree-based widget for editing keyboard shortcuts with conflict detection.

#include <dsfw/widgets/WidgetsGlobal.h>

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
}

namespace dsfw::widgets {

/// @brief Describes a single shortcut entry for the editor.
struct DSFW_WIDGETS_API ShortcutEntry {
    QString displayName;       ///< @brief Human-readable shortcut name.
    QString category;          ///< @brief Category for grouping in the tree.
    const char *settingsKeyPath; ///< @brief Settings key path for persistence.
    QString defaultSequence;   ///< @brief Default key sequence string.
};

/// @brief Tree-based keyboard shortcut editor with conflict detection.
class DSFW_WIDGETS_API ShortcutEditorWidget : public QWidget {
    Q_OBJECT

public:
    /// @brief Construct a shortcut editor widget.
    /// @param settings Application settings instance.
    /// @param entries List of shortcut entries to edit.
    /// @param parent Parent widget.
    explicit ShortcutEditorWidget(dstools::AppSettings *settings,
                                  const std::vector<ShortcutEntry> &entries,
                                  QWidget *parent = nullptr);
    ~ShortcutEditorWidget() override;

    /// @brief Show the shortcut editor as a modal dialog.
    /// @param settings Application settings instance.
    /// @param entries List of shortcut entries to edit.
    /// @param parent Parent widget for the dialog.
    static void showDialog(dstools::AppSettings *settings,
                           const std::vector<ShortcutEntry> &entries,
                           QWidget *parent);

    /// @brief Commit any in-progress key sequence edit.
    void commitPendingEdit();

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

    QKeySequenceEdit *m_activeEditor = nullptr;
    QTreeWidgetItem *m_activeItem = nullptr;
    int m_activeEntryIndex = -1;

    std::vector<QTreeWidgetItem *> m_entryItems;
};

} // namespace dsfw::widgets
