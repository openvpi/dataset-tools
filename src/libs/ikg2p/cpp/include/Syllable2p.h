#ifndef SYLLABLE2P_H
#define SYLLABLE2P_H

#include <QObject>

namespace IKg2p
{
    class Syllable2pPrivate;

    class Syllable2p : public QObject
    {
        Q_OBJECT

        Q_DECLARE_PRIVATE(Syllable2p)

    public:
        explicit Syllable2p(QString phonemeDict, QString sep1 = "\t", QString sep2 = " ", QObject* parent = nullptr);

        ~Syllable2p() override;

        [[nodiscard]] QStringList syllableToPhoneme(const QString& syllable) const;

        [[nodiscard]] QList<QStringList> syllableToPhoneme(const QStringList& syllables) const;

    protected:
        explicit Syllable2p(Syllable2pPrivate& d, QObject* parent = nullptr);

        QScopedPointer<Syllable2pPrivate> d_ptr;
    };
}


#endif // SYLLABLE2P_H
