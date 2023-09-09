
#pragma once

#include <QString>
#include <qjsonstream.h>

struct MinLabelCfg {
    QString open;
    QString exportAudio;
    QString next;
    QString prev;
    QString play;

    QString rootDir;
};
QAS_JSON_NS(MinLabelCfg);