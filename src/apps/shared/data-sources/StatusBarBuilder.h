#pragma once

#include <QHBoxLayout>
#include <QLabel>
#include <QObject>

namespace dstools {

    class StatusBarBuilder {
    public:
        explicit StatusBarBuilder(QWidget *container) : m_container(container) {
            m_layout = new QHBoxLayout(container);
            m_layout->setContentsMargins(0, 0, 0, 0);
        }

        QLabel *addLabel(const QString &text, int stretch = 0) {
            auto *label = new QLabel(text, m_container);
            m_layout->addWidget(label, stretch);
            return label;
        }

        template <typename Sender, typename Signal, typename Widget, typename Func>
        QMetaObject::Connection connect(Sender *sender, Signal signal, Widget *widget, Func &&func) {
            return QObject::connect(sender, signal, widget, std::forward<Func>(func));
        }

        QWidget *container() const {
            return m_container;
        }

    private:
        QWidget *m_container;
        QHBoxLayout *m_layout;
    };

} // namespace dstools