
#pragma once

#include <QString>
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

    DsSentence() {}
    DsSentence(const QString &text, const QString &ph_seq, const QString &ph_dur, const QString &ph_num,
               const QString &note_seq, const QString &note_dur, const QString &note_slur, const QString &f0_seq,
               const QString &f0_timestep)
        : text(text), ph_seq(ph_seq), ph_dur(ph_dur), ph_num(ph_num), note_seq(note_seq), note_dur(note_dur),
          note_slur(note_slur), f0_seq(f0_seq), f0_timestep(f0_timestep) {
    }
};
QAS_JSON_NS(DsSentence);