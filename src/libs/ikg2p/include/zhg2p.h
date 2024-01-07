#ifndef ZHG2P_H
#define ZHG2P_H

#include <QObject>

namespace IKg2p {
    enum class errorType { Default = 0, Ignore = 1 };

    class ZhG2pPrivate;

    class ZhG2p : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ZhG2p)
    public:
        explicit ZhG2p(QString language, QObject *parent = nullptr);
        ~ZhG2p();

        QString convert(const QString &input, bool tone = true, bool convertNum = true,
                        errorType error = errorType::Default);
        QString convert(const QList<QStringView> &input, bool tone = true, bool convertNum = true,
                        errorType error = errorType::Default);

        QString tradToSim(const QString &text) const;
        bool isPolyphonic(const QString &text) const;
        QStringList getDefaultPinyin(const QString &text) const;

    protected:
        ZhG2p(ZhG2pPrivate &d, QObject *parent = nullptr);

        QScopedPointer<ZhG2pPrivate> d_ptr;
    };

}

#endif // ZHG2P_H