//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_Q_PROPERTY_H
#define DATASET_TOOLS_Q_PROPERTY_H

#define Q_PROPERTY_DECLARE(TYPE, prop, Prop)                                                       \
private:                                                                                           \
    Q_PROPERTY(TYPE prop READ prop WRITE set##Prop NOTIFY prop##Changed)                           \
    TYPE m_##prop;                                                                                 \
                                                                                                   \
public:                                                                                            \
    inline TYPE prop() const {                                                                     \
        return m_##prop;                                                                           \
    }                                                                                              \
                                                                                                   \
    inline void set##Prop(const TYPE &prop) {                                                      \
        m_##prop = prop;                                                                           \
        update();                                                                                  \
        emit prop##Changed();                                                                      \
    }                                                                                              \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void prop##Changed();                                                                          \
                                                                                                   \
private:

// Definition
#define Q_D_PROPERTY(TYPE, prop, Prop)                                                             \
private:                                                                                           \
    Q_PROPERTY(TYPE prop READ prop WRITE set##Prop NOTIFY prop##Changed)                           \
                                                                                                   \
public:                                                                                            \
    TYPE prop() const;                                                                             \
    void set##Prop(const TYPE &prop);                                                              \
                                                                                                   \
Q_SIGNALS:                                                                                         \
    void prop##Changed();                                                                          \
                                                                                                   \
private:

// Declaration
#define Q_D_PROPERTY_DECLARE(TYPE, prop, Prop, Class)                                              \
    TYPE Class::prop() const {                                                                     \
        Q_D(const Class);                                                                          \
        return d->prop;                                                                            \
    }                                                                                              \
                                                                                                   \
    void Class::set##Prop(const TYPE &prop) {                                                      \
        Q_D(Class);                                                                                \
        d->prop = prop;                                                                            \
        emit prop##Changed();                                                                      \
    }


#endif // DATASET_TOOLS_Q_PROPERTY_H
