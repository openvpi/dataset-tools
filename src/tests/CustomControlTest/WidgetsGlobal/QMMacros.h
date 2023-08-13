//
// Created by fluty on 2023/8/13.
//

#ifndef DATASET_TOOLS_QMMACROS_H
#define DATASET_TOOLS_QMMACROS_H


/**
 * @brief Enter once
 *
 */
#define Q_ENTER_ONCE                                                                                                   \
    {                                                                                                                  \
        static bool _entered = false;                                                                                  \
        if (_entered) {                                                                                                \
            return;                                                                                                    \
        }                                                                                                              \
        _entered = true;                                                                                               \
    }


/**
 * @brief Concat string
 *
 */
#define Q_STRCAT(A, B) A##B

/**
 * @brief Traverse directory, create local "d" var
 *
 */
#define Q_D_EXPLORE(str)                                                                                               \
    QFileInfoList d;                                                                                                   \
    {                                                                                                                  \
        QDir dir;                                                                                                      \
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);                                              \
        dir.setPath(str);                                                                                              \
        d = dir.entryInfoList();                                                                                       \
    }

/**
 * @brief Define singleton members
 *
 */
#define Q_SINGLETON(T)                                                                                                 \
private:                                                                                                               \
    static T *self;                                                                                                    \
                                                                                                                       \
private:                                                                                                               \
    T *construct() {                                                                                                   \
        Q_ASSERT(!self);                                                                                               \
        self = this;                                                                                                   \
        return self;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    void destruct() {                                                                                                  \
        self = nullptr;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
public:                                                                                                                \
    static T *instance() {                                                                                             \
        return self;                                                                                                   \
    }

/**
 * @brief Declare static singleton members
 *
 */
#define Q_SINGLETON_DECLARE(T) T *T::self = nullptr;

/**
 * @brief Define codec members
 *
 */
#define Q_CHARSET_INSTANCE                                                                                             \
protected:                                                                                                             \
    QTextCodec *m_codec;                                                                                               \
    bool m_charsetDetermined;                                                                                          \
                                                                                                                       \
public:                                                                                                                \
    QTextCodec *codec() const;                                                                                         \
    void setCodec(QTextCodec *codec);                                                                                  \
                                                                                                                       \
    bool charsetDetermined() const;

/**
 * @brief Define static codec members
 *
 */
#define Q_CHARSET_STATIC                                                                                               \
private:                                                                                                               \
    static QTextCodec *s_codeForDefault;                                                                               \
                                                                                                                       \
public:                                                                                                                \
    static QTextCodec *codeForDefault();                                                                               \
    static void setCodeForDefault(QTextCodec *codec);

/**
 * @brief Declare codec members
 *
 */
#define Q_CHARSET_INSTANCE_DECLARE(T)                                                                                  \
    QTextCodec *T::codec() const {                                                                                     \
        return m_codec;                                                                                                \
    }                                                                                                                  \
                                                                                                                       \
    void T::setCodec(QTextCodec *codec) {                                                                              \
        m_codec = codec;                                                                                               \
    }                                                                                                                  \
                                                                                                                       \
    bool T::charsetDetermined() const {                                                                                \
        return m_charsetDetermined;                                                                                    \
    }

/**
 * @brief Declare static codec members
 *
 */
#define Q_CHARSET_STATIC_DECLARE(T)                                                                                    \
    QTextCodec *T::s_codeForDefault = nullptr;                                                                         \
                                                                                                                       \
    QTextCodec *T::codeForDefault() {                                                                                  \
        return s_codeForDefault;                                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    void T::setCodeForDefault(QTextCodec *codec) {                                                                     \
        s_codeForDefault = codec;                                                                                      \
    }

#define Q_CHARSET                                                                                                      \
    Q_CHARSET_INSTANCE                                                                                                 \
    Q_CHARSET_STATIC

#define Q_CHARSET_DECLARE(T)                                                                                           \
    Q_CHARSET_INSTANCE_DECLARE(T)                                                                                      \
    Q_CHARSET_STATIC_DECLARE(T)

/**
 * @brief Define unique id members
 *
 */
#define Q_UNIQUE_ID(T)                                                                                                 \
public:                                                                                                                \
    void initId() {                                                                                                    \
        m_id = ++s_idMax;                                                                                              \
        s_idMap.insert(m_id, this);                                                                                    \
    }                                                                                                                  \
                                                                                                                       \
    void destructId() {                                                                                                \
        s_idMap.remove(m_id);                                                                                          \
    }                                                                                                                  \
                                                                                                                       \
    int id() const {                                                                                                   \
        return m_id;                                                                                                   \
    }                                                                                                                  \
                                                                                                                       \
    static T *LookUp(int id);                                                                                          \
                                                                                                                       \
protected:                                                                                                             \
    int m_id;                                                                                                          \
                                                                                                                       \
    static int s_idMax;                                                                                                \
    static QMap<int, T *> s_idMap;

/**
 * @brief Declare static unique id members
 *
 */
#define Q_UNIQUE_ID_DECLARE(T)                                                                                         \
    int T::s_idMax = 0;                                                                                                \
    QMap<int, T *> T::s_idMap;                                                                                         \
    T *T::LookUp(int id) {                                                                                             \
        auto it = s_idMap.find(id);                                                                                    \
        if (it == s_idMap.end()) {                                                                                     \
            return nullptr;                                                                                            \
        }                                                                                                              \
        return it.value();                                                                                             \
    }

#endif // DATASET_TOOLS_QMMACROS_H
