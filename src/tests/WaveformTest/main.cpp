//
// Created by fluty on 2023/8/27.
//

#include <QApplication>
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

#include "NoteGraphicsItem.h"
#include "PianoRollBackgroundGraphicsItem.h"
#include "PianoRollGraphicsScene.h"
#include "PianoRollGraphicsView.h"
#include "TracksController.h"
#include "TracksGraphicsView.h"

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);
    QApplication::setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

    QApplication::setEffectEnabled(Qt::UI_AnimateTooltip, false);

    QMainWindow w;
    w.setStyleSheet(QString("QMainWindow { background: #232425 }"));

    auto btnOpenAudioFile = new QPushButton;
    btnOpenAudioFile->setText("Add...");

    auto tracksController = new TracksController;

    QObject::connect(btnOpenAudioFile, &QPushButton::clicked, tracksController, [&]() {
        auto fileName = QFileDialog::getOpenFileName(
            btnOpenAudioFile, "Select an Audio File", ".",
            "All Audio File (*.wav *.flac *.mp3);;Wave File (*.wav);;Flac File (*.flac);;MP3 File (*.mp3)");
        if (fileName.isNull())
            return;

        tracksController->addAudioClipToNewTrack(fileName);
    });

    auto pianoRollScene = new PianoRollGraphicsScene;
    auto pianoRollView = new PianoRollGraphicsView;
    pianoRollView->setMinimumHeight(150);
    pianoRollView->setScene(pianoRollScene);
    pianoRollView->centerOn(0, 0);
    pianoRollView->setStyleSheet("QGraphicsView { border: none }");
    QObject::connect(pianoRollView, &TracksGraphicsView::scaleChanged, pianoRollScene, &TracksGraphicsScene::setScale);

    auto gridItem = new PianoRollBackgroundGraphicsItem;
    QObject::connect(pianoRollView, &TracksGraphicsView::visibleRectChanged, gridItem,
                     &TimeGridGraphicsItem::onVisibleRectChanged);
    QObject::connect(pianoRollView, &TracksGraphicsView::scaleChanged, gridItem, &TimeGridGraphicsItem::setScale);
    pianoRollScene->addItem(gridItem);

    auto loadProjectFile = [](const QString &filename, QJsonObject *jsonObj) {
        QFile loadFile(filename);
        if (!loadFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open project file";
            return false;
        }
        QByteArray allData = loadFile.readAll();
        loadFile.close();
        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(allData, &err);
        if (err.error != QJsonParseError::NoError)
            return false;
        if (json.isObject()) {
            *jsonObj = json.object();
        }
        return true;
    };

    class Note {
    public:
        int start;
        int length;
        int keyIndex;
        QString lyric;
    };

    auto loadNotes = [](const QJsonObject &obj) {
        auto arrTracks = obj.value("tracks").toArray();
        auto firstTrack = arrTracks.first().toObject();
        auto arrPatterns = firstTrack.value("patterns").toArray();
        auto firstPattern = arrPatterns.first().toObject();
        auto notes = firstPattern.value("notes").toArray();

        auto decodeNotes = [](const QJsonArray &arrNotes) {
            QList<Note> notes;
            for (const auto valNote : qAsConst(arrNotes)) {
                auto objNote = valNote.toObject();
                Note note;
                note.start = objNote.value("pos").toInt();
                note.length = objNote.value("dur").toInt();
                note.keyIndex = objNote.value("pitch").toInt();
                note.lyric = objNote.value("lyric").toString();
                notes.append(note);
            }
            return notes;
        };

        return decodeNotes(notes);
    };

    auto filename = "E:/Test/Param/小小.json";
    QJsonObject jsonObj;
    if (loadProjectFile(filename, &jsonObj)) {
        auto notes = loadNotes(jsonObj);
        int noteId = 0;
        for (const auto &note : notes) {
            auto noteItem = new NoteGraphicsItem(0);
            noteItem->setStart(note.start);
            noteItem->setLength(note.length);
            noteItem->setKeyIndex(note.keyIndex);
            noteItem->setLyric(note.lyric);

            noteItem->setVisibleRect(pianoRollView->visibleRect());
            noteItem->setScaleX(pianoRollView->scaleX());
            noteItem->setScaleY(pianoRollView->scaleY());
            pianoRollScene->addItem(noteItem);
            QObject::connect(pianoRollView, &PianoRollGraphicsView::scaleChanged, noteItem,
                             &NoteGraphicsItem::setScale);
            QObject::connect(pianoRollView, &PianoRollGraphicsView::visibleRectChanged, noteItem,
                             &NoteGraphicsItem::setVisibleRect);
            noteId++;
        }
    }

    auto splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(tracksController->tracksView());
    splitter->addWidget(pianoRollView);

    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(btnOpenAudioFile);
    mainLayout->addWidget(splitter);
    // mainLayout->addWidget(tracksController->tracksView());
    // mainLayout->addWidget(pianoRollView);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}