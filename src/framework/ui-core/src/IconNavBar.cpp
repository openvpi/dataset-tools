#include <dsfw/IconNavBar.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

namespace dsfw {

IconNavBar::IconNavBar(QWidget *parent) : QWidget(parent) {
    setFixedWidth(m_barWidth);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
}

int IconNavBar::iconSize() const {
    return m_iconSize;
}

void IconNavBar::setIconSize(int size) {
    m_iconSize = size;
    m_barWidth = m_iconSize + 40;
    setFixedWidth(m_barWidth);
    update();
}

int IconNavBar::barWidth() const {
    return m_barWidth;
}

int IconNavBar::addItem(const QIcon &icon, const QString &label) {
    m_items.append({icon, label});
    const int idx = m_items.size() - 1;
    setMinimumHeight(m_items.size() * ItemHeight);
    update();
    return idx;
}

int IconNavBar::count() const {
    return m_items.size();
}

int IconNavBar::currentIndex() const {
    return m_currentIndex;
}

void IconNavBar::setCurrentIndex(int index) {
    if (index < 0 || index >= m_items.size() || index == m_currentIndex)
        return;
    m_currentIndex = index;
    update();
    emit currentChanged(index);
}

void IconNavBar::paintEvent(QPaintEvent * /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto &pal = palette();
    p.fillRect(rect(), pal.color(QPalette::Window));

    const QFont labelFont(font().family(), 7);
    const QFontMetrics labelFm(labelFont);

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        const QRect itemRect(0, i * ItemHeight, m_barWidth, ItemHeight);

        // Highlight selected
        if (i == m_currentIndex) {
            p.fillRect(itemRect, pal.color(QPalette::Highlight));
        }

        const auto &item = m_items[i];

        // Icon centered horizontally, top portion of item
        const int iconX = (m_barWidth - m_iconSize) / 2;
        const int iconY = itemRect.y() + 6;
        const QRect iconRect(iconX, iconY, m_iconSize, m_iconSize);

        const auto mode = (i == m_currentIndex) ? QIcon::Active : QIcon::Normal;
        item.icon.paint(&p, iconRect, Qt::AlignCenter, mode);

        // Label below icon
        if (!item.label.isEmpty()) {
            p.setFont(labelFont);
            const auto textColor = (i == m_currentIndex)
                                       ? pal.color(QPalette::HighlightedText)
                                       : pal.color(QPalette::WindowText);
            p.setPen(textColor);

            const int textY = iconY + m_iconSize + 2;
            const QRect textRect(0, textY, m_barWidth, ItemHeight - (textY - itemRect.y()));
            const auto elidedText =
                labelFm.elidedText(item.label, Qt::ElideRight, m_barWidth - 4);
            p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, elidedText);
        }
    }
}

void IconNavBar::mousePressEvent(QMouseEvent *event) {
    const int idx = static_cast<int>(event->position().y()) / ItemHeight;
    if (idx >= 0 && idx < m_items.size()) {
        setCurrentIndex(idx);
    }
}

} // namespace dsfw
