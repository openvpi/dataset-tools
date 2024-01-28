//
// Created by fluty on 2024/1/27.
//

#include "DsNote.h"

int DsNote::start() const {
    return m_start;
}
void DsNote::setStart(int start) {
    m_start = start;
}
int DsNote::length() const {
    return m_length;
}
void DsNote::setLength(int length) {
    m_length = length;
}
int DsNote::keyIndex() const {
    return m_keyIndex;
}
void DsNote::setKeyIndex(int keyIndex) {
    m_keyIndex = keyIndex;
}
QString DsNote::lyric() const {
    return m_lyric;
}
void DsNote::setLyric(const QString &lyric) {
    m_lyric = lyric;
}
QString DsNote::pronunciation() const {
    return m_pronunciation;
}
void DsNote::setPronunciation(const QString &pronunciation) {
    m_pronunciation = pronunciation;
}