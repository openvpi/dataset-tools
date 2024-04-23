#ifndef ChineseG2p_H
#define ChineseG2p_H

#include <QObject>

namespace IKg2p
{
    enum class errorType
    {
        Default = 0, Ignore = 1
    };

    class ChineseG2pPrivate;

    class ChineseG2p : public QObject
    {
        Q_OBJECT
        Q_DECLARE_PRIVATE(ChineseG2p)

    public:
        explicit ChineseG2p(QString language, QObject* parent = nullptr);

        ~ChineseG2p() override;

        QStringList hanziToPinyin(const QString& input, bool tone = true, bool convertNum = true,
                                  errorType error = errorType::Default);

        QStringList hanziToPinyin(const QStringList& input, bool tone = true, bool convertNum = true,
                                  errorType error = errorType::Default);

        [[nodiscard]] QString tradToSim(const QString& text) const;

        [[nodiscard]] bool isPolyphonic(const QString& text) const;

        [[nodiscard]] QStringList getDefaultPinyin(const QString& text, bool tone = true) const;

    protected:
        explicit ChineseG2p(ChineseG2pPrivate& d, QObject* parent = nullptr);

        QScopedPointer<ChineseG2pPrivate> d_ptr;

    private:
        QStringList hanziToPinyin(const QList<QStringView>& input, bool tone = true, bool convertNum = true,
                                  errorType error = errorType::Default);
    };
}

#endif // ChineseG2p_H
