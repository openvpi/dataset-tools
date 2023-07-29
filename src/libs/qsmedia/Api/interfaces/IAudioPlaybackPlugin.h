#ifndef IAUDIOPLAYBACKPLUGIN_H
#define IAUDIOPLAYBACKPLUGIN_H

#include <QObject>

#include "QsMediaGlobal.h"

QSAPI_BEGIN_NAMESPACE

class IAudioPlayback;

class QSMEDIA_API IAudioPlaybackPlugin : public QObject {
    Q_OBJECT
public:
    explicit IAudioPlaybackPlugin(QObject *parent = nullptr);
    ~IAudioPlaybackPlugin();

    virtual IAudioPlayback *create(const QString &key, QObject *parent) = 0;
};

QSAPI_END_NAMESPACE

#define QsApi_IAudioPlaybackPlugin_IID "org.ChorusKit.QsLib.IAudioPlaybackPlugin"

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QsApi::IAudioPlaybackPlugin, QsApi_IAudioPlaybackPlugin_IID)
QT_END_NAMESPACE

#endif // IAUDIOPLAYBACKPLUGIN_H
