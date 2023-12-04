//
// Created by FlutyDeer on 2023/12/1.
//

#ifndef TRACKSMODEL_H
#define TRACKSMODEL_H

#include <QColor>
#include <QList>
#include <QString>

class TracksModel {
public:
    // definations
    class TrackControl {
    public:
        double gain() const;
        void setGain(double gain);
        double pan() const;
        void setPan(double pan);
        bool mute() const;
        void setMute(bool mute);
        bool solo() const;
        void setSolo(bool solo);

    private:
        double m_gain = 1.0;
        double m_pan = 0;
        bool m_mute = false;
        bool m_solo = false;
    };

    class Clip {
    public:
        enum ClipType { Audio, Singing };

        int clipId;

        virtual ClipType type() = 0;
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

    class AudioClip : public Clip {
    public:
        ClipType type() override {
            return Audio;
        }
        QString path() const;
        void setPath(const QString &path);

    private:
        QString m_path;
    };

    class Note {
    public:
        int start();
        void setStart(int start);
        int length();
        void setLength(int length);
        int keyIndex();
        void setKeyIndex(int keyIndex);
        QString lyric();
        void setLyric(const QString &lyric);

    private:
        int m_start = 0;
        int m_length = 480;
        int m_keyIndex = 60;
        QString m_lyric;
    };

    // class RealParamCurve {
    // public:
    //     int offset();
    //     void setOffset(int offset);
    //     QList<float> values() const;
    //     void setValues(const QList<float> &values);
    //
    // private:
    //     int m_offset = 0; // tick
    //     QList<float> m_values;
    // };
    //
    // class RealParam {
    // public:
    //     RealParamCurve original() const;
    //     void setOriginal(const RealParamCurve &original);
    //     RealParamCurve edited() const;
    //     void setEdited(const RealParamCurve &edited);
    //
    // private:
    //     RealParamCurve m_original;
    //     RealParamCurve m_edited;
    // };
    //
    // class Param {
    // public:
    //     RealParam pitch() const;
    //
    // protected:
    //     RealParam m_pitch;
    //     // RealParam m_energy;
    //     // RealParam m_breathiness;
    // };

    class SingingClip : public Clip {
    public:
        ClipType type() override {
            return Singing;
        }
        QList<Note> notes() const;
        void setNotes(const QList<Note> &notes);
        // void addNote(const Note &note);
        // void removeNote(const Note &note);
        // void clearNotes();

    private:
        QList<Note> m_notes;
        // QList<RealParam> m_params;
    };

    class Color {
    public:
        // int colorIndex = 0;
        // QColor customColor = QColor();
        static QColor background() {
            return QColor(112, 156, 255);
        }
        // QColor foreground();
        // QColor accent();
    };


    class Track {
    public:
        QString name() const;
        void setName(const QString &name);
        TrackControl control() const;
        void setControl(const TrackControl &control);
        QList<Clip *> clips;
        // QList<Clip *> clips() const;
        // void setClips(const QList<Clip> &clips);
        void addClip(Clip *clip);
        // void removeClip(const Clip &clip);
        void removeClip(int index);

        Color color() const;
        void setColor(const Color &color);

    private:
        QString m_name;
        TrackControl m_control = TrackControl();
        // QList<Clip *> m_clips;
        Color m_color;
    };

    // menbers
    QList<Track> tracks = QList<Track>();

    TracksModel();
    ~TracksModel();
};



#endif // TRACKSMODEL_H
