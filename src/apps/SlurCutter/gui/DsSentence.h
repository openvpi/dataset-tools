#pragma once

#include <QString>

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <utility>


namespace SlurCutter {
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
        QString note_glide;

        DsSentence() = default;
        DsSentence(QString text, QString ph_seq, QString ph_dur, QString ph_num, QString note_seq, QString note_dur,
                   QString note_slur, QString f0_seq, QString f0_timestep, QString note_glide);
    };

    inline DsSentence::DsSentence(QString text, QString ph_seq, QString ph_dur, QString ph_num, QString note_seq,
                                  QString note_dur, QString note_slur, QString f0_seq, QString f0_timestep,
                                  QString note_glide)
        : text(std::move(text)), ph_seq(std::move(ph_seq)), ph_dur(std::move(ph_dur)), ph_num(std::move(ph_num)),
          note_seq(std::move(note_seq)), note_dur(std::move(note_dur)), note_slur(std::move(note_slur)),
          f0_seq(std::move(f0_seq)), f0_timestep(std::move(f0_timestep)), note_glide(std::move(note_glide)) {
    }

    inline DsSentence loadDsSentencesFromJsonObj(const QJsonObject &content) {
        DsSentence sentence;
        sentence.text = content.value("text").toString();
        sentence.ph_seq = content.value("ph_seq").toString();
        sentence.ph_dur = content.value("ph_dur").toString();
        sentence.ph_num = content.value("ph_num").toString();
        sentence.note_seq = content.value("note_seq").toString();
        sentence.note_dur = content.value("note_dur").toString();
        sentence.note_slur = content.value("note_slur").toString();
        sentence.f0_seq = content.value("f0_seq").toString();
        sentence.f0_timestep = content.value("f0_timestep").toString();
        sentence.note_glide = content.value("note_glide").toString();
        return sentence;
    }
}