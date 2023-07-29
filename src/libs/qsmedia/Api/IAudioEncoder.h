#ifndef IAUDIOENCODER_H
#define IAUDIOENCODER_H

#include <QObject>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class QSMEDIA_API IAudioEncoder : public QObject {
    Q_OBJECT
public:
    explicit IAudioEncoder(QObject *parent = nullptr);
    ~IAudioEncoder();
};

QSAPI_END_NAMESPACE

#define QsApi_IAudioEncoder_IID "org.ChorusKit.QsLib.IAudioEncoder"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QsApi::IAudioEncoder, QsApi_IAudioEncoder_IID)
QT_END_NAMESPACE

#endif // IAUDIOENCODER_H
