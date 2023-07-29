#ifndef IAUDIODECODERPLUGIN_H
#define IAUDIODECODERPLUGIN_H

#include <QObject>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class IAudioDecoder;

class QSMEDIA_API IAudioDecoderPlugin : public QObject {
    Q_OBJECT
public:
    explicit IAudioDecoderPlugin(QObject *parent = nullptr);
    ~IAudioDecoderPlugin();

    virtual IAudioDecoder *create(const QString &key, QObject *parent) = 0;
};

QSAPI_END_NAMESPACE

#define QsApi_IAudioDecoderPlugin_IID "org.ChorusKit.QsLib.IAudioDecoderPlugin"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QsApi::IAudioDecoderPlugin, QsApi_IAudioDecoderPlugin_IID)
QT_END_NAMESPACE

#endif // IAUDIODECODERPLUGIN_H
