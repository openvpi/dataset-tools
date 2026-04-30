#pragma once

/// @file IconNavBar.h
/// @brief Narrow icon sidebar navigation widget for page switching.

#include <QIcon>
#include <QString>
#include <QVector>
#include <QWidget>

namespace dsfw {

/// @brief Vertical icon-based navigation sidebar widget.
///
/// Renders a narrow column of icons; clicking an icon emits currentChanged().
/// Used by AppShell for page navigation.
class IconNavBar : public QWidget {
    Q_OBJECT
public:
    /// @brief Construct the navigation bar.
    /// @param parent Optional parent widget.
    explicit IconNavBar(QWidget *parent = nullptr);

    /// @brief Add a navigation item.
    /// @param icon Icon to display.
    /// @param label Tooltip text.
    /// @return Index of the newly added item.
    int addItem(const QIcon &icon, const QString &label);
    /// @brief Return the number of items.
    /// @return Item count.
    int count() const;
    /// @brief Return the index of the selected item.
    /// @return Current index, or -1 if none selected.
    int currentIndex() const;
    /// @brief Set the selected item by index.
    /// @param index Item index.
    void setCurrentIndex(int index);

signals:
    /// @brief Emitted when the selected item changes.
    /// @param index New selected index.
    void currentChanged(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct Item {
        QIcon icon;
        QString label;
    };
    QVector<Item> m_items;
    int m_currentIndex = -1;
    static constexpr int ItemHeight = 48;
    static constexpr int IconSize = 24;
    static constexpr int NavWidth = 64;
};

} // namespace dsfw
