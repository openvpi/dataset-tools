//
// Created by fluty on 2023/9/5.
//

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QScrollBar>

#include "NoteGraphicsItem.h"
#include "ParamEditArea.h"
#include "ParamEditGraphicsView.h"
#include "ParamModel.h"
#include "PianoRollGraphicsView.h"

#include <QFile>
#include <QJsonDocument>
#include <QSplitter>

int main(int argc, char *argv[]) {
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication a(argc, argv);

    auto f = QFont("Microsoft Yahei UI", 9);
    f.setHintingPreference(QFont::PreferNoHinting);
    QApplication::setFont(f);

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

    QJsonObject jsonObj;
//    auto filename = "D:/Test/Param/test.json";
    auto filename = "D:/Test/Param/小小.json";
    loadProjectFile(filename, &jsonObj);

    auto paramModel = ParamModel::load(jsonObj);

    QMainWindow w;

    auto editArea = new ParamEditArea;
//    editArea->loadParam(paramModel.realEnergy[0]);

    const int quarterNoteWidth = 62;
    const int quarterNoteHeight = 24;
    int sceneHeight = 6 * 12 * quarterNoteHeight;
    int sceneWidth = 3840;

    auto pianoRollScene = new QGraphicsScene;

//    for (int i = 0; i < 4; i++) {
//        auto noteItem = new NoteGraphicsItem();
//        noteItem->setRect(QRectF(i * quarterNoteWidth, 24 * i, quarterNoteWidth, quarterNoteHeight));
//        noteItem->setText("喵");
//        pianoRollScene->addItem(noteItem);
//    }

    for (int i = 0; i < paramModel.notes.count(); i++) {
        auto note = paramModel.notes[i];
        auto noteItem = new NoteGraphicsItem();
        auto noteLeft = note.start * quarterNoteWidth / 480;
        auto noteTop = (107 - note.keyIndex) * 24;
        auto noteWidth = note.length * quarterNoteWidth / 480;
        noteItem->setRect(QRectF(noteLeft, noteTop, noteWidth, quarterNoteHeight));
        noteItem->setText(note.lyric);
        pianoRollScene->addItem(noteItem);
    }

    auto lastNote = paramModel.notes.last();
    auto lastNoteEndPos = lastNote.start + lastNote.length;
    auto newSceneWidth = lastNoteEndPos * quarterNoteWidth / 480 + 192;
    if (newSceneWidth > sceneWidth)
        sceneWidth = newSceneWidth;

    pianoRollScene->setSceneRect(0, 0, sceneWidth, sceneHeight);

    auto firstNote = paramModel.notes.first();
    auto firstNoteLeft = firstNote.start * quarterNoteWidth / 480;
    auto firstNoteTop = (107 - firstNote.keyIndex) * 24;

//    pianoRollScene->addItem(noteItem);
    auto pianoRollView = new PianoRollGraphicsView;
    pianoRollView->setScene(pianoRollScene);
    pianoRollView->centerOn(firstNoteLeft, firstNoteTop);

    auto paramEditScene = new QGraphicsScene;
    paramEditScene->setSceneRect(0, 0, sceneWidth, 200);

    QPen pen;
    pen.setWidthF(1.5);
    auto drawParams = [&](QList<ParamModel::RealParamCurve> curves) {
        for (const auto &curve : curves) {
            QPainterPath path;
            auto firstValue = curve.values.first();
            path.moveTo(0, firstValue < 1 ? 150 * (1 - firstValue) : 0);
            int i = 0;
            for (const auto value : qAsConst(curve.values)) {
                auto x = (curve.offset + i) * quarterNoteWidth / 480.0;
                auto y = value < 1 ? 150 * (1 - value) : 0;
                path.lineTo(x, y);
                i++;
            }

            auto pathItem = new QGraphicsPathItem(path);
            pathItem->setPen(pen);
            paramEditScene->addItem(pathItem);
        }
    };

    auto realEnergy = paramModel.realEnergy;
    auto realBreathiness = paramModel.realBreathiness;

//    pen.setColor(QColor(255, 175, 95));
//    drawParams(realEnergy);
//    pen.setColor(QColor(112, 156, 255));
//    drawParams(realBreathiness);

//    auto curve = paramModel.realEnergy.first();
//    QPainterPath path;
//    auto firstValue = curve.values.first();
//    path.moveTo(0, firstValue < 1 ? 200 * (1 - firstValue) : 0);
//    int i = 0;
//    for (const auto value : qAsConst(curve.values)) {
//        auto x = (curve.offset + i) * quarterNoteWidth / 480;
//        auto y = value < 1 ? 200 * (1 - value) : 0;
//        path.lineTo(x, y);
//        i++;
//    }
//
//    auto pathItem = QGraphicsPathItem(path);
//    paramEditScene->addItem(&pathItem);

    auto paramEditView = new ParamEditGraphicsView;
    paramEditView->setMinimumHeight(150);
    paramEditView->setScene(paramEditScene);
    paramEditView->centerOn(firstNoteLeft, firstNoteTop);
    paramEditView->setStyleSheet("QGraphicsView { border: none }");

    QObject::connect(pianoRollView->horizontalScrollBar(), &QScrollBar::valueChanged, paramEditView, [=](int value){
        paramEditView->horizontalScrollBar()->setValue(value);
    });

    auto splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
//    splitter->addWidget(pianoRollView);
    splitter->addWidget(editArea);
//    splitter->addWidget(paramEditView);

    auto mainLayout = new QVBoxLayout;
//    mainLayout->addWidget(editArea);
    mainLayout->addWidget(splitter);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}