#pragma once

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

struct DSFW_WIDGETS_API ShortcutEntry {
    QString displayName;
    QString category;
    const char *settingsKeyPath;
    QString defaultSequence;
};

class DSFW_WIDGETS_API ShortcutEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ShortcutEditorWidget(dstools::AppSettings *settings,
                                  const std::vector<ShortcutEntry> &entries,
                                  QWidget *parent = nullptr);
    ~ShortcutEditorWidget() override;

    static void showDialog(dstools::AppSettings *settings,
                           const std::vector<ShortcutEntry> &entries,
                           QWidget *parent);

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
