#include <dsfw/widgets/PageBanner.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace dsfw::widgets {

static QString bannerTypeIcon(BannerType type) {
    switch (type) {
    case BannerType::Info:     return QStringLiteral("\342\204\271");  // ℹ
    case BannerType::Warning:  return QStringLiteral("\342\232\240");  // ⚠
    case BannerType::Error:    return QStringLiteral("\342\235\214");  // ❌
    case BannerType::Success:  return QStringLiteral("\342\234\205");  // ✅
    case BannerType::Progress: return QStringLiteral("\360\237\224\204"); // 🔄
    }
    return {};
}

static QString bannerTypeStyle(BannerType type) {
    switch (type) {
    case BannerType::Info:
        return QStringLiteral("background: #E3F2FD; color: #1565C0; border: 1px solid #90CAF9;");
    case BannerType::Warning:
        return QStringLiteral("background: #FFF3E0; color: #C62828; border: 1px solid #FFCC80;");
    case BannerType::Error:
        return QStringLiteral("background: #FFEBEE; color: #B71C1C; border: 1px solid #EF9A9A;");
    case BannerType::Success:
        return QStringLiteral("background: #E8F5E9; color: #1B5E20; border: 1px solid #A5D6A7;");
    case BannerType::Progress:
        return QStringLiteral("background: #E8EAF6; color: #283593; border: 1px solid #9FA8DA;");
    }
    return {};
}

PageBanner::PageBanner(QWidget *parent)
    : QFrame(parent) {
    setupUi();
    setVisible(false);
}

PageBanner::~PageBanner() = default;

void PageBanner::setupUi() {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(12, 6, 12, 6);
    m_layout->setSpacing(8);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedWidth(20);
    m_layout->addWidget(m_iconLabel);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_layout->addWidget(m_messageLabel, 1);

    m_buttonLayout = new QHBoxLayout();
    m_buttonLayout->setSpacing(4);
    m_layout->addLayout(m_buttonLayout);

    auto *closeBtn = new QPushButton(QStringLiteral("\303\227"), this); // ×
    closeBtn->setFixedSize(24, 24);
    closeBtn->setFlat(true);
    closeBtn->setStyleSheet(QStringLiteral("border: none; font-size: 16px; font-weight: bold;"));
    connect(closeBtn, &QPushButton::clicked, this, &PageBanner::closeRequested);
    m_layout->addWidget(closeBtn);
}

void PageBanner::show(BannerType type, const QString &message,
                      const QStringList &buttonLabels) {
    applyStyle(type);
    m_iconLabel->setText(bannerTypeIcon(type));
    m_messageLabel->setText(message);

    while (m_buttonLayout->count() > 0) {
        auto *item = m_buttonLayout->takeAt(0);
        delete item->widget();
        delete item;
    }

    for (int i = 0; i < buttonLabels.size(); ++i) {
        auto *btn = new QPushButton(buttonLabels[i], this);
        btn->setFlat(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            QStringLiteral("background: rgba(0,0,0,0.08); padding: 2px 10px; "
                           "border-radius: 3px; font-weight: bold;"));
        connect(btn, &QPushButton::clicked, this, [this, i] {
            emit buttonClicked(i);
        });
        m_buttonLayout->addWidget(btn);
    }

    setVisible(true);
}

void PageBanner::hide() {
    setVisible(false);
}

void PageBanner::applyStyle(BannerType type) {
    setStyleSheet(QStringLiteral("PageBanner { %1 border-radius: 4px; }")
                      .arg(bannerTypeStyle(type)));
    setFrameShape(QFrame::StyledPanel);
}

} // namespace dsfw::widgets