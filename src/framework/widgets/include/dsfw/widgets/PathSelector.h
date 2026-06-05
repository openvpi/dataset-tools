#pragma once
/// @file PathSelector.h
/// @brief File/directory path selector widget with browse button and drag-and-drop.

#include <QWidget>
#include <dsfw/widgets/WidgetsGlobal.h>

class QLabel;
class QLineEdit;
class QPushButton;

namespace dsfw::widgets {

    /// @brief Widget for selecting a file or directory path with browse and drag-and-drop support.
    class DSFW_WIDGETS_API PathSelector : public QWidget {
        Q_OBJECT
        Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    public:
        /// @brief Selection mode.
        enum Mode { OpenFile, SaveFile, Directory };

        /// @brief Construct a PathSelector.
        /// @param mode Selection mode (open file, save file, or directory).
        /// @param label Display label text.
        /// @param filter File filter string for file dialogs.
        /// @param parent Parent widget.
        /// @param settingsKey Key for remembering the last browsed directory (empty = no memory).
        explicit PathSelector(Mode mode, const QString &label, const QString &filter = {}, QWidget *parent = nullptr,
                              const QString &settingsKey = {});

        /// @brief Set the default suffix for save file dialogs.
        /// @param suffix Default file suffix (e.g., "wav").
        void setDefaultSuffix(const QString &suffix);

        /// @brief Set the dialog title (overrides label text in file dialogs).
        /// @param title Dialog title text.
        void setDialogTitle(const QString &title);

        /// @brief Get the currently selected path.
        /// @return Selected path string.
        QString path() const;

        /// @brief Set the path.
        /// @param path Path string to set.
        void setPath(const QString &path);

        /// @brief Set placeholder text for the line edit.
        /// @param text Placeholder text.
        void setPlaceholder(const QString &text);

        /// @brief Get the selection mode.
        /// @return Current mode.
        Mode mode() const;

        /// @brief Get the underlying line edit widget.
        /// @return Pointer to the QLineEdit.
        QLineEdit *lineEdit() const;

    signals:
        /// @brief Emitted when the selected path changes.
        /// @param path New path string.
        void pathChanged(const QString &path);

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;

    private:
        void onBrowseClicked();
        void saveRecentDir(const QString &path);

        Mode m_mode;
        QString m_filter;
        QString m_settingsKey;
        QString m_defaultSuffix;
        QString m_dialogTitle;
        QLabel *m_label;
        QLineEdit *m_lineEdit;
        QPushButton *m_browseBtn;
    };

} // namespace dsfw::widgets
