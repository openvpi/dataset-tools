#ifndef ENG2P_H
#define ENG2P_H


#include <QObject>

namespace IKg2p {

    class EnG2pPrivate;

    class EnG2p : public QObject {
        Q_OBJECT
        Q_DECLARE_PRIVATE(EnG2p)
    public:
        explicit EnG2p(QObject *parent = nullptr);
        ~EnG2p() override;

        QString word2arpabet(const QString &word) const;
        QString word2arpabet(const QStringList &wordList) const;

    protected:
        explicit EnG2p(EnG2pPrivate &d, QObject *parent = nullptr);

        QScopedPointer<EnG2pPrivate> d_ptr;
    };

}



#endif // ENG2P_H
