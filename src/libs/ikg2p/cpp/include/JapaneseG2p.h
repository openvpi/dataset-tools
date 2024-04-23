#ifndef JPG2P_H
#define JPG2P_H

#include <QObject>

namespace IKg2p
{
    class JpG2pPrivate;

    class JapaneseG2p final : public QObject
    {
        Q_OBJECT
        Q_DECLARE_PRIVATE(JpG2p)

    public:
        explicit JapaneseG2p(QObject* parent = nullptr);
        ~JapaneseG2p() override;

        [[nodiscard]] QStringList kanaToRomaji(const QString& kanaStr, const bool& doubleWrittenSokuon = false) const;
        [[nodiscard]] QStringList kanaToRomaji(const QStringList& kanaList,
                                               const bool& doubleWrittenSokuon = false) const;

        [[nodiscard]] QStringList romajiToKana(const QString& romajiStr) const;
        [[nodiscard]] QStringList romajiToKana(const QStringList& romajiList) const;

    protected:
        explicit JapaneseG2p(JpG2pPrivate& d, QObject* parent = nullptr);

        QScopedPointer<JpG2pPrivate> d_ptr;
    };
}

#endif // JPG2P_H
