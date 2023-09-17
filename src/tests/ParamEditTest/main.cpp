//
// Created by fluty on 2023/9/5.
//

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>

#include "ParamEditArea.h"
#include "ParamGraphicsView.h"
#include "ParamModel.h"
#include "NoteGraphicsItem.h"

#include <QFile>
#include <QJsonDocument>

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
    auto filename = "D:/Test/Param/test.json";
    loadProjectFile(filename, &jsonObj);

    auto paramModel = ParamModel::load(jsonObj);

    QMainWindow w;

    auto editArea = new ParamEditArea;
    editArea->loadParam(paramModel.realBreathiness);

    const int quarterNoteWidth = 48;
    const int quarterNoteHeight = 24;
    int sceneHeight = 6 * 12 * quarterNoteHeight;

//    auto noteItem = new NoteGraphicsItem();
//    noteItem->setRect(QRectF(0, 0, quarterNoteWidth, quarterNoteHeight));
//    noteItem->setText("喵");
    auto scene = new QGraphicsScene(0, 0, 48000, sceneHeight);

    for (int i = 0; i < 4; i++) {
//        auto rectItem = new QGraphicsRectItem(i * quarterNoteWidth , 48 * i + 64, quarterNoteWidth, quarterNoteHeight);
        auto noteItem = new NoteGraphicsItem();
        noteItem->setRect(QRectF(i * quarterNoteWidth, 24 * i, quarterNoteWidth, quarterNoteHeight));
        noteItem->setText("喵");
        scene->addItem(noteItem);
//        scene->addItem(rectItem);
    }

//    scene->addItem(noteItem);
    auto view = new ParamGraphicsView;
    view->setRenderHint(QPainter::Antialiasing);
    view->setScene(scene);
    view->centerOn(0, 0);

    auto mainLayout = new QVBoxLayout;
//    mainLayout->addWidget(editArea);
    mainLayout->addWidget(view);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 600);
    w.show();

    return QApplication::exec();
}