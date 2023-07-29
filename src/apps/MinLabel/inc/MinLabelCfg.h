
#pragma once

#include <QString>
#include <qjsonstream.h>

struct MinLabelCfg {
    QString open;
    QString next;
    QString prev;
    QString play;

    bool preserveText;

    MinLabelCfg() : preserveText(false) {}
};
QAS_JSON_NS(MinLabelCfg);