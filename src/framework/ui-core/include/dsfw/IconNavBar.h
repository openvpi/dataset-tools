#pragma once

#include <QIcon>
#include <QString>
#include <QVector>
#include <QWidget>

namespace dsfw {

class IconNavBar : public QWidget {
    Q_OBJECT
public:
    explicit IconNavBar(QWidget *parent = nullptr);

    int addItem(const QIcon &icon, const QString &label);
    int count() const;
    int currentIndex() const;
    void setCurrentIndex(int index);

signals:
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
