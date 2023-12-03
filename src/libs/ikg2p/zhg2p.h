#ifndef ZHG2P_H
#define ZHG2P_H

#include <QObject>

namespace IKg2p {

    class ZhG2pPrivate;

    class ZhG2p : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ZhG2p)
    public:
        explicit ZhG2p(QString language, QObject *parent = nullptr);
        ~ZhG2p();

        QString convert(const QString &input, bool tone = true, bool covertNum = true);
        QString convert(const QList<QStringView> &input, bool tone = true, bool covertNum = true);

    protected:
        ZhG2p(ZhG2pPrivate &d, QObject *parent = nullptr);

        QScopedPointer<ZhG2pPrivate> d_ptr;
    };

}

#endif // ZHG2P_H