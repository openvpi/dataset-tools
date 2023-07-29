
#pragma once

#include <QString>
#include <qjsonstream.h>

struct SlurCutterCfg {
    QString open;
    QString next;
    QString prev;
    QString play;

    bool snapToKeys;
    bool showPitchTextOverlay;

    SlurCutterCfg() : snapToKeys(true), showPitchTextOverlay(false) {}
};
QAS_JSON_NS(SlurCutterCfg);