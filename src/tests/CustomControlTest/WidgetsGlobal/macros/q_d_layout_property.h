//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_Q_D_LAYOUT_PROPERTY_H
#define DATASET_TOOLS_Q_D_LAYOUT_PROPERTY_H

// Definition
#define Q_D_LAYOUT_PROPERTY(LL, CC)                                                                \
private:                                                                                           \
    Q_PROPERTY(QMargins LL##LayoutMargins READ LL##LayoutMargins WRITE                             \
                   set##CC##LayoutMargins NOTIFY LL##LayoutChanged)                                \
    Q_PROPERTY(QPixelSize LL##LayoutSpacing READ LL##LayoutSpacing WRITE                           \
                   set##CC##LayoutSpacing NOTIFY LL##LayoutChanged)                                \
public:                                                                                            \
    QMargins LL##LayoutMargins() const;                                                            \
    void set##CC##LayoutMargins(const QMargins &margins);                                          \
                                                                                                   \
    QPixelSize LL##LayoutSpacing() const;                                                          \
    void set##CC##LayoutSpacing(const QPixelSize &spacing);                                        \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void LL##LayoutChanged();                                                                      \
                                                                                                   \
private:

// Declaration
#define Q_D_LAYOUT_PROPERTY_DECLARE(LL, CC, NAME, Class)                                           \
    QMargins Class::LL##LayoutMargins() const {                                                    \
        Q_D(const Class);                                                                          \
        return d->NAME->contentsMargins();                                                         \
    }                                                                                              \
                                                                                                   \
    void Class::set##CC##LayoutMargins(const QMargins &margins) {                                  \
        Q_D(Class);                                                                                \
        d->NAME->setContentsMargins(margins);                                                      \
        emit LL##LayoutChanged();                                                                  \
    }                                                                                              \
                                                                                                   \
    QPixelSize Class::LL##LayoutSpacing() const {                                                  \
        Q_D(const Class);                                                                          \
        return QPixelSize(d->NAME->spacing());                                                     \
    }                                                                                              \
                                                                                                   \
    void Class::set##CC##LayoutSpacing(const QPixelSize &spacing) {                                \
        Q_D(Class);                                                                                \
        d->NAME->setSpacing(spacing.value());                                                      \
        emit LL##LayoutChanged();                                                                  \
    }


#endif // DATASET_TOOLS_Q_D_LAYOUT_PROPERTY_H
