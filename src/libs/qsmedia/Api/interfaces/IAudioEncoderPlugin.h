#ifndef IAUDIOENCODERPLUGIN_H
#define IAUDIOENCODERPLUGIN_H

#include <QObject>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class IAudioEncoder;

class QSMEDIA_API IAudioEncoderPlugin : public QObject {
    Q_OBJECT
public:
    explicit IAudioEncoderPlugin(QObject *parent = nullptr);
    ~IAudioEncoderPlugin();

    virtual IAudioEncoder *create(const QString &key, QObject *parent) = 0;
};

QSAPI_END_NAMESPACE

#define QsApi_IAudioEncoderPlugin_IID "org.ChorusKit.QsLib.IAudioEncoderPlugin"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QsApi::IAudioEncoderPlugin, QsApi_IAudioEncoderPlugin_IID)
QT_END_NAMESPACE

#endif // IAUDIOENCODERPLUGIN_H
