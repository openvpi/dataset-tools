#include "MainWindow.h"

#include <QApplication>

#include <QPainter>
#include <QStyleOptionTab>
#include <QStylePainter>

#include <QDir>
#include <QMessageBox>

#include "../utils/DmlGpuUtils.h"

#include <iostream>

#include <QJsonDocument>
#include <QLineEdit>
#include <qjsonstream.h>

static bool isDmlAvailable(int &recommendedIndex) {
    const GpuInfo recommendedGpu = DmlGpuUtils::getRecommendedGpu();

    if (recommendedGpu.index == -1) {
        return false;
    }

    recommendedIndex = recommendedGpu.index;

    return recommendedGpu.memory > 0;
}

static void saveConfig(const QString &keyConfPath, const SomeCfg &cfg) {
    QFile file(keyConfPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(qAsClassToJson(cfg)).toJson());
        file.close();
    } else {
        QMessageBox::critical(nullptr, QApplication::applicationName(), "Failed to create config file.");
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    mainLayout = new QHBoxLayout();

    const QString modelFolder = QDir::cleanPath(
#ifdef Q_OS_MAC
        QApplication::applicationDirPath() + "/../Resources/some_model"
#else
        QApplication::applicationDirPath() + QDir::separator() + "some_model"
#endif
    );

    auto rmProvider = Some::ExecutionProvider::CPU;
    int device_id = -1;
    if (isDmlAvailable(device_id)) {
        rmProvider = Some::ExecutionProvider::DML;
        std::cout << "DML is available. Recommended GPU index: " << device_id << std::endl;
    } else {
        std::cout << "DML is not available." << std::endl;
    }


    if (QDir(modelFolder).exists()) {
        const auto modelPath = modelFolder + QDir::separator() + "model.onnx";
        if (!QFile(modelPath).exists())
            QMessageBox::information(this, "Warning",
                                     "Missing model.onnx, please read ReadMe.md and download the model again.");
        else {
            const std::filesystem::path somePath = modelPath
#ifdef _WIN32
                                                       .toStdWString();
#else
                                                       .toStdString();
#endif
            m_some = std::make_shared<Some::Some>(somePath, rmProvider, device_id);
            if (!m_some->is_open())
                QMessageBox::critical(this, "Warning", "Failed to create SOME session. Make sure onnx model is valid.");
        }
    } else {
#ifdef Q_OS_MAC
        QMessageBox::information(
            this, "Warning",
            "Please read ReadMe.md and download asrModel to unzip to app bundle's Resources directory.");
#else
        QMessageBox::information(this, "Warning",
                                 "Please read ReadMe.md and download SomeModel to unzip to the root directory.");
#endif
    }

    const QString keyConfPath = QApplication::applicationDirPath() + "/some_config.json";
    try {
        if (QApplication::arguments().contains("--reset-keys")) {
            throw std::exception();
        }

        QFile file(keyConfPath);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::exception();
        }

        QJsonParseError err{};
        const QJsonDocument cfgDoc = QJsonDocument::fromJson(file.readAll(), &err);

        file.close();

        if (err.error != QJsonParseError::NoError || !cfgDoc.isObject()) {
            throw std::exception();
        }

        QAS::JsonStream stream(cfgDoc.object());
        stream >> cfg;
    } catch (const std::exception &e) {
    }

    parentWidget = new QTabWidget();

    // const auto batchWidget = new QWidget();
    const auto midiWidget = new MidiWidget(m_some, &cfg);
    // parentWidget->addTab(batchWidget, "制作数据集");
    parentWidget->addTab(midiWidget, "wav转midi");

    setCentralWidget(parentWidget);
    resize(600, 300);
}

MainWindow::~MainWindow() {
    saveConfig(QApplication::applicationDirPath() + "/some_config.json", cfg);
};