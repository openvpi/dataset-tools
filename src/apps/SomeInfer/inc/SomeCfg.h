#pragma once

#include <QString>
#include <qjsonstream.h>

struct SomeCfg {
    QString wavPath = "";
    QString outMidiPath = "";
    QString tempo = "120";
};
QAS_JSON_NS(SomeCfg);