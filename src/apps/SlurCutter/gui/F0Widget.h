
#pragma once

#include "intervaltree.hpp"
#include "SlurCutterCfg.h"
#include <QFrame>
#include <QMenu>
#include <QScrollBar>
#include <tuple>


class F0Widget : public QFrame {
    Q_OBJECT
public:
    explicit F0Widget(QWidget *parent = nullptr);
    ~F0Widget();

    void setDsSentenceContent(const QJsonObject &content);
    void setErrorStatusText(const QString &text);
    void loadConfig(const SlurCutterCfg &cfg);
    void pullConfig(SlurCutterCfg &cfg) const;

    void clear();

    struct ReturnedDsString {
        QString note_seq;
        QString note_dur;
        QString note_slur;
    };
    ReturnedDsString getSavedDsStrings();
    bool empty();

public slots:
    void setPlayheadPos(double pos);

signals:
    void requestReloadSentence();

public:
    static int NoteNameToMidiNote(const QString &noteName);
    static QString MidiNoteToNoteName(int midiNote);
    static double FrequencyToMidiNote(double f);
    static std::tuple<int, double> PitchToNoteAndCents(double pitch);
    static QString PitchToNotePlusCentsString(double pitch);

protected:
    struct MiniNote;

    // Protected methods
    std::tuple<size_t, size_t> refF0IndexRange(double startTime, double endTime) const;
    bool mouseOnNote(const QPoint &mousePos, Intervals::Interval<double, MiniNote> *returnNote = nullptr) const;

    // Convenience methods
    double pitchOnWidgetY(int y) const;
    double timeOnWidgetX(int x) const;
    void setNoteContextMenuEntriesEnabled();

    // Data manipulation methods
    void splitNoteUnderMouse();
    void shiftDraggedNoteByPitch(double pitchDelta);
    void setDraggedNotePitch(int pitch);

protected slots:
    // Data manip (global)
    void convertAllRestsToNormal();

    // (Note)
    void mergeCurrentSlurToLeftNode(bool checked);
    void toggleCurrentNoteRest();

protected:

    // Events
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    // Stored DS file data
    struct MiniNote {
        double duration;
        int pitch;    // Semitone from A0
        double cents; // nan if no cent deviation
        QString text;
        bool isSlur, isRest;

        // Required by IntervalTree
        bool operator<(const MiniNote &other) const {
            return duration < other.duration;
        }
    };
    bool isEmpty = true;
    using MiniNoteInterval = Intervals::Interval<double, MiniNote>;
    Intervals::IntervalTree<double, MiniNote> midiIntervals;
    Intervals::IntervalTree<double> markerIntervals;
    QVector<double> f0Values;
    double f0Timestep;

    // Size constants
    static constexpr int KeyWidth = 60, ScrollBarWidth = 25;
    static constexpr int TimeAxisHeight = 40, HorizontalScrollHeight = ScrollBarWidth,
                         TimelineHeight = TimeAxisHeight + HorizontalScrollHeight;
    static constexpr int NotePadding = 4;

    // Display state
    double semitoneHeight = 30.0, secondWidth = 200.0;
    double centerPitch = 60.0, centerTime = 0.0;
    std::tuple<double, double> pitchRange = {0.0, 0.0}, timeRange = {0.0, 0.0};
    double playheadPos = 0.0;

    bool clampPitchToF0Bounds = true; // Also affects scroll bar range // TODO: INCOMPLETE
    bool snapToKey = true;
    bool showPitchTextOverlay = false;

    bool hasError;
    QString errorStatusText;

    // Data Manipulation State
    QPoint draggingStartPos = {-1, -1};
    Qt::MouseButton draggingButton = Qt::MouseButton::NoButton;
    std::tuple<double, double> draggingNoteInterval;
    MiniNoteInterval contextMenuNoteInterval;
    enum {
        None,
        Note
    } draggingMode;
    bool dragging = false;
    bool draggingNoteInCents = false;
    double draggingNoteStartPitch = 0.0, draggingNoteBeginCents = 0, draggingNoteBeginPitch = 0;

    // Embedded widgets
    QScrollBar *horizontalScrollBar, *verticalScrollBar;
    
    QMenu *bgMenu;
    QAction *bgMenuReloadSentence;
    QAction *bgMenu_OptionPrompt;
    QAction *bgMenu_CautionPrompt;
    QAction *bgMenuSnapByDefault;
    QAction *bgMenuShowPitchTextOverlay;
    QAction *bgMenuConvRestsToNormal;

    QMenu *noteMenu;
    QAction *noteMenuMergeLeft;
    QAction *noteMenuToggleRest;

private:
    // Private unified methods
    void setTimeAxisCenterAndSyncScrollbar(double time);
    void setF0CenterAndSyncScrollbar(double pitch);

private slots:
    void onHorizontalScroll(int value);
    void onVerticalScroll(int value);
};
