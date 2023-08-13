//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_Q_LAYOUT_PROPERTY_H
#define DATASET_TOOLS_Q_LAYOUT_PROPERTY_H

#define Q_LAYOUT_PROPERTY_DELCARE                                                                  \
private:                                                                                           \
    Q_PROPERTY(                                                                                    \
        QMargins layoutMargins READ layoutMargins WRITE setLayoutMargins NOTIFY layoutChanged)     \
    Q_PROPERTY(                                                                                    \
        QPixelSize layoutSpacing READ layoutSpacing WRITE setLayoutSpacing NOTIFY layoutChanged)   \
public:                                                                                            \
    inline QMargins layoutMargins() const {                                                        \
        auto m = QWidget::layout();                                                                \
        return m ? m->contentsMargins() : QMargins();                                              \
    }                                                                                              \
                                                                                                   \
    inline void setLayoutMargins(const QMargins &margins) {                                        \
        auto m = QWidget::layout();                                                                \
        m ? m->setContentsMargins(margins) : void(0);                                              \
        emit layoutChanged();                                                                      \
    }                                                                                              \
                                                                                                   \
    inline QPixelSize layoutSpacing() const {                                                      \
        auto m = QWidget::layout();                                                                \
        return QPixelSize(m ? m->spacing() : 0);                                                   \
    }                                                                                              \
                                                                                                   \
    inline void setLayoutSpacing(const QPixelSize &spacing) {                                      \
        auto m = QWidget::layout();                                                                \
        m ? m->setSpacing(spacing.value()) : void(0);                                              \
        emit layoutChanged();                                                                      \
    }                                                                                              \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void layoutChanged();                                                                          \
                                                                                                   \
private:

#define Q_PRIVATE_LAYOUT_PROPERTY_DELCARE(LL, CC, m)                                               \
private:                                                                                           \
    Q_PROPERTY(QMargins LL##LayoutMargins READ LL##LayoutMargins WRITE                             \
                   set##CC##LayoutMargins NOTIFY LL##LayoutChanged)                                \
    Q_PROPERTY(QPixelSize LL##LayoutSpacing READ LL##LayoutSpacing WRITE                           \
                   set##CC##LayoutSpacing NOTIFY LL##LayoutChanged)                                \
public:                                                                                            \
    inline QMargins LL##LayoutMargins() const {                                                    \
        return m->contentsMargins();                                                               \
    }                                                                                              \
                                                                                                   \
    inline void set##CC##LayoutMargins(const QMargins &margins) {                                  \
        m->setContentsMargins(margins);                                                            \
        emit LL##LayoutChanged();                                                                  \
    }                                                                                              \
                                                                                                   \
    inline QPixelSize LL##LayoutSpacing() const {                                                  \
        return QPixelSize(m->spacing());                                                           \
    }                                                                                              \
                                                                                                   \
    inline void set##CC##LayoutSpacing(const QPixelSize &spacing) {                                \
        m->setSpacing(spacing.value());                                                            \
        emit LL##LayoutChanged();                                                                  \
    }                                                                                              \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void LL##LayoutChanged();                                                                      \
                                                                                                   \
private:


#endif // DATASET_TOOLS_Q_LAYOUT_PROPERTY_H
