//
// Created by fluty on 2024/1/27.
//

#include "DsAudioClip.h"


QString DsAudioClip::path() const {
    return m_path;
}
void DsAudioClip::setPath(const QString &path) {
    m_path = path;
}