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

//    auto rectItem = new QGraphicsRectItem(QRectF(0, 0, 200, 200));
//    auto textItem = new QGraphicsTextItem("Hello");
    auto noteItem = new NoteGraphicsItem();
    noteItem->setRect(QRectF(0, 0, 48, 24));
    noteItem->setText("喵");
    auto scene = new QGraphicsScene;
//    scene->setSceneRect(QRectF(0, 0, 250, 250));
//    scene->addItem(rectItem);
//    scene->addItem(textItem);
    scene->addItem(noteItem);
    auto view = new ParamGraphicsView;
    view->setRenderHint(QPainter::Antialiasing);
    view->setScene(scene);

    auto mainLayout = new QVBoxLayout;
//    mainLayout->addWidget(editArea);
    mainLayout->addWidget(view);

    auto mainWidget = new QWidget;
    mainWidget->setLayout(mainLayout);

    w.setCentralWidget(mainWidget);
    w.resize(1200, 300);
    w.show();

    return QApplication::exec();
}