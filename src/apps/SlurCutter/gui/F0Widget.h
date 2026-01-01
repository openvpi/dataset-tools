
#pragma once

#include "intervaltree.hpp"

#include <QFrame>
#include <QMenu>
#include <QScrollBar>
#include <QSettings>
#include <QtWidgets/qactiongroup.h>
#include <tuple>

namespace SlurCutter {
    class F0Widget final : public QFrame {
        Q_OBJECT
    public:
        explicit F0Widget(QWidget *parent = nullptr);
        ~F0Widget() override;

        void setDsSentenceContent(const QJsonObject &content);
        void setErrorStatusText(const QString &text);
        void loadConfig(const QSettings *cfg);
        void pullConfig(QSettings &cfg) const;

        void clear();

        struct ReturnedDsString {
            QString note_seq;
            QString note_dur;
            QString note_slur;
            QString note_glide;
        };
        ReturnedDsString getSavedDsStrings() const;
        bool empty() const;

    public slots:
        void setPlayHeadPos(double pos);

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
        enum class GlideStyle {
            None,
            Up,
            Down,
        };

        // Protected methods
        std::tuple<size_t, size_t> refF0IndexRange(double startTime, double endTime) const;
        bool mouseOnNote(const QPoint &mousePos, Intervals::Interval<double, MiniNote> *returnNote = nullptr) const;

        // Convenience methods
        double pitchOnWidgetY(int y) const;
        double timeOnWidgetX(int x) const;
        void setNoteContextMenuEntriesEnabled() const;

        // Data manipulation methods
        void splitNoteUnderMouse();
        void shiftDraggedNoteByPitch(double pitchDelta);
        void setDraggedNotePitch(int pitch);
        void setDraggedNoteGlide(GlideStyle style);

    protected slots:
        // Data manip (global)
        void modeChanged();
        void convertAllRestsToNormal();

        // (Note)
        void setMenuFromCurrentNote() const;
        void mergeCurrentSlurToLeftNode(bool checked);
        void toggleCurrentNoteRest();
        void setCurrentNoteGlideType(const QAction *action);

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
        struct MiniPhoneme {
            QString ph;
            double begin;
            double duration;
        };
        struct MiniNote {
            double duration;
            int pitch;    // Semitone from A0
            double cents; // nan if no cent deviation
            QString text;
            QVector<MiniPhoneme> phonemes;
            bool isSlur, isRest;
            GlideStyle glide;

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
        static constexpr int CrosshairTextMargin = 8, CrosshairTextPadding = 5;

        // Display state
        double semitoneHeight = 30.0, secondWidth = 200.0;
        double centerPitch = 60.0, centerTime = 0.0;
        std::tuple<double, double> pitchRange = {0.0, 0.0}, timeRange = {0.0, 0.0};
        double playheadPos = 0.0;

        bool clampPitchToF0Bounds = true; // Also affects scroll bar range // TODO: INCOMPLETE
        bool snapToKey = true;
        bool showPitchTextOverlay = false;
        bool showPhonemeTexts = true;
        bool showCrosshairAndPitch = true;

        bool hasError;
        QString errorStatusText;

        // Data Manipulation State
        QPoint draggingStartPos = {-1, -1};
        Qt::MouseButton draggingButton = Qt::MouseButton::NoButton;
        std::tuple<double, double> draggingNoteInterval;
        MiniNoteInterval contextMenuNoteInterval;
        enum {
            None,
            Note,
            Glide,
        } draggingMode,
            selectedDragMode;
        bool dragging = false;
        bool draggingNoteInCents = false;
        double draggingNoteStartPitch = 0.0, draggingNoteBeginCents = 0, draggingNoteBeginPitch = 0;

        // Embedded widgets
        QScrollBar *horizontalScrollBar, *verticalScrollBar;

        QMenu *bgMenu;
        QAction *bgMenuReloadSentence;
        QAction *bgMenu_CautionPrompt;
        QAction *bgMenuConvRestsToNormal;
        QAction *bgMenu_OptionPrompt;
        QAction *bgMenuSnapByDefault;
        QAction *bgMenuShowPitchTextOverlay;
        QAction *bgMenuShowPhonemeTexts;
        QAction *bgMenuShowCrosshairAndPitch;
        QAction *bgMenu_ModePrompt;
        QAction *bgMenuModeNote;
        QAction *bgMenuModeGlide;

        QActionGroup *bgMenuModeGroup;

        QMenu *noteMenu;
        QAction *noteMenuMergeLeft;
        QAction *noteMenuToggleRest;

        QAction *noteMenuGlidePrompt;
        QActionGroup *noteMenuSetGlideType;
        QAction *noteMenuSetGlideNone;
        QAction *noteMenuSetGlideUp;
        QAction *noteMenuSetGlideDown;

    private:
        // Private unified methods
        void setTimeAxisCenterAndSyncScrollbar(double time);
        void setF0CenterAndSyncScrollbar(double pitch);

    private slots:
        void onHorizontalScroll(int value);
        void onVerticalScroll(int value);
    };
}