//
// Created by FlutyDeer on 2023/12/1.
//

#include "TracksModel.h"

double TracksModel::TrackControl::gain() const {
    return m_gain;
}
void TracksModel::TrackControl::setGain(double gain) {
    m_gain = gain;
}
double TracksModel::TrackControl::pan() const {
    return m_pan;
}
void TracksModel::TrackControl::setPan(double pan) {
    m_pan = pan;
}
bool TracksModel::TrackControl::mute() const {
    return m_mute;
}
void TracksModel::TrackControl::setMute(bool mute) {
    m_mute = mute;
}
bool TracksModel::TrackControl::solo() const {
    return m_solo;
}
void TracksModel::TrackControl::setSolo(bool solo) {
    m_solo = solo;
}
QString TracksModel::Clip::name() const {
    return m_name;
}
void TracksModel::Clip::setName(const QString &text) {
    m_name = text;
}
int TracksModel::Clip::start() const {
    return m_start;
}
void TracksModel::Clip::setStart(int start) {
    m_start = start;
}
int TracksModel::Clip::length() const {
    return m_length;
}
void TracksModel::Clip::setLength(int length) {
    m_length = length;
}
int TracksModel::Clip::clipStart() const {
    return m_clipStart;
}
void TracksModel::Clip::setClipStart(int clipStart) {
    m_clipStart = clipStart;
}
int TracksModel::Clip::clipLen() const {
    return m_clipLen;
}
void TracksModel::Clip::setClipLen(int clipLen) {
    m_clipLen = clipLen;
}
double TracksModel::Clip::gain() const {
    return m_gain;
}
void TracksModel::Clip::setGain(double gain) {
    m_gain = gain;
}
bool TracksModel::Clip::mute() const {
    return m_mute;
}
void TracksModel::Clip::setMute(bool mute) {
    m_mute = mute;
}
QString TracksModel::AudioClip::path() const {
    return m_path;
}
void TracksModel::AudioClip::setPath(const QString &path) {
    m_path = path;
}
int TracksModel::Note::start() {
    return m_start;
}
void TracksModel::Note::setStart(int start) {
    m_start = start;
}
int TracksModel::Note::length() {
    return m_length;
}
void TracksModel::Note::setLength(int length) {
    m_length = length;
}
int TracksModel::Note::keyIndex() {
    return m_keyIndex;
}
void TracksModel::Note::setKeyIndex(int keyIndex) {
    m_keyIndex = keyIndex;
}
QString TracksModel::Note::lyric() {
    return m_lyric;
}
void TracksModel::Note::setLyric(const QString &lyric) {
    m_lyric = lyric;
}
QList<TracksModel::Note> TracksModel::SingingClip::notes() const {
    return m_notes;
}
void TracksModel::SingingClip::setNotes(const QList<Note> &notes) {
    m_notes = notes;
}
QString TracksModel::Track::name() const {
    return m_name;
}
void TracksModel::Track::setName(const QString &name) {
    m_name = name;
}
TracksModel::TrackControl TracksModel::Track::control() const {
    return m_control;
}
void TracksModel::Track::setControl(const TrackControl &control) {
    m_control = control;
}
// QList<TracksModel::Clip *> TracksModel::Track::clips() const {
//     return m_clips;
// }
// void TracksModel::Track::addClip(Clip *clip) {
//     m_clips.append(clip);
//     // Sort by start time
// }
// void TracksModel::Track::removeClip(int index) {
//     m_clips.removeAt(index);
// }
TracksModel::Color TracksModel::Track::color() const {
    return m_color;
}
void TracksModel::Track::setColor(const Color &color) {
    m_color = color;
}
TracksModel::TracksModel() {
}
TracksModel::~TracksModel() {
}