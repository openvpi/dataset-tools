#pragma once

#include <QFrame>
#include <QString>
#include <QStringList>
#include <dsfw/widgets/WidgetsGlobal.h>

class QHBoxLayout;
class QLabel;
class QPushButton;

namespace dsfw::widgets {

    enum class BannerType { Info, Warning, Error, Success, Progress };

    class DSFW_WIDGETS_API PageBanner : public QFrame {
        Q_OBJECT
    public:
        explicit PageBanner(QWidget *parent = nullptr);
        ~PageBanner() override;

        void show(BannerType type, const QString &message, const QStringList &buttonLabels = {});
        void hide();

    signals:
        void buttonClicked(int index);
        void closeRequested();

    private:
        void setupUi();
        void applyStyle(BannerType type);

        QHBoxLayout *m_layout = nullptr;
        QLabel *m_iconLabel = nullptr;
        QLabel *m_messageLabel = nullptr;
        QHBoxLayout *m_buttonLayout = nullptr;
    };

} // namespace dsfw::widgets