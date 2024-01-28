//
// Created by fluty on 2024/1/27.
//

#ifndef DSCLIP_H
#define DSCLIP_H

#include <QSharedPointer>

#include "DsNote.h"
#include "DsParams.h"

class DsClip {
public:
    enum ClipType { Audio, Singing, Generic };

    virtual ~DsClip() = default;

    int clipId;

    virtual ClipType type() const {
        return Generic;
    }
    QString name() const;
    void setName(const QString &text);
    int start() const;
    void setStart(int start);
    int length() const;
    void setLength(int length);
    int clipStart() const;
    void setClipStart(int clipStart);
    int clipLen() const;
    void setClipLen(int clipLen);
    double gain() const;
    void setGain(double gain);
    bool mute() const;
    void setMute(bool mute);

protected:
    QString m_name;
    int m_start = 0;
    int m_length = 0;
    int m_clipStart = 0;
    int m_clipLen = 0;
    double m_gain = 0;
    bool m_mute = false;
};

class DsAudioClip final : public DsClip {
public:
    ClipType type() const override {
        return Audio;
    }
    QString path() const;
    void setPath(const QString &path);

private:
    QString m_path;
};

class DsSingingClip final : public DsClip {
public:
    ClipType type() const override {
        return Singing;
    }

    // QList<DsNote> notes() const;
    // void setNotes(const QList<DsNote> &notes);
    // void addNote(const Note &note);
    // void removeNote(const Note &note);
    // void clearNotes();
    QList<DsNote> notes;
    DsParams params;

private:
    QList<DsNote> m_notes;
    // DsParams m_params;
};

using DsClipPtr = QSharedPointer<DsClip>;
using DsSingingClipPtr = QSharedPointer<DsSingingClip>;
using DsAudioClipPtr = QSharedPointer<DsAudioClip>;

#endif // DSCLIP_H
