
#pragma once

#include <QString>
#include <QtCore/qstring.h>
#include <qjsonstream.h>

struct DsSentence {
    QString text;
    QString ph_seq;
    QString ph_dur;
    QString ph_num;
    QString note_seq;
    QString note_dur;
    QString note_slur;
    QString f0_seq;
    QString f0_timestep;
    QString note_ornament;

    DsSentence() {}
    DsSentence(const QString &text, const QString &ph_seq, const QString &ph_dur, const QString &ph_num,
               const QString &note_seq, const QString &note_dur, const QString &note_slur, const QString &f0_seq,
               const QString &f0_timestep, const QString &note_ornament)
        : text(text), ph_seq(ph_seq), ph_dur(ph_dur), ph_num(ph_num), note_seq(note_seq), note_dur(note_dur),
          note_slur(note_slur), f0_seq(f0_seq), f0_timestep(f0_timestep), note_ornament(note_ornament) {}
};
QAS_JSON_NS(DsSentence);