#ifndef IAUDIODECODER_H
#define IAUDIODECODER_H

#include <QObject>
#include <QVariantMap>

#include "NAudio/WaveStream.h"
#include "QsMediaNamespace.h"

QSAPI_BEGIN_NAMESPACE

class QSMEDIA_API IAudioDecoder : public QObject, public NAudio::WaveStream {
    Q_OBJECT
public:
    explicit IAudioDecoder(QObject *parent = nullptr);
    ~IAudioDecoder();

public:
    virtual bool open(const QString &filename, const QsMedia::WaveArguments &args = {}) = 0;
    virtual void close() = 0;

    virtual bool isOpen() const = 0;

    /**
     * @brief The original format of the processed audio
     *
     * @return NAudio::WaveFormat
     */
    virtual NAudio::WaveFormat inFormat() const = 0;
};

QSAPI_END_NAMESPACE

#define QsApi_IAudioDecoder_IID "org.ChorusKit.QsLib.IAudioDecoder"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QsApi::IAudioDecoder, QsApi_IAudioDecoder_IID)
QT_END_NAMESPACE

#endif // IAUDIODECODER_H
