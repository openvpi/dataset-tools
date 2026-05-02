#include <QApplication>
#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>

#include <dsfw/widgets/PathSelector.h>
#include <dsfw/widgets/ViewportController.h>
#include <dsfw/widgets/RunProgressRow.h>
#include <dsfw/widgets/FileProgressTracker.h>
#include <dsfw/widgets/PropertyEditor.h>
#include <dsfw/widgets/LogViewer.h>
#include <dsfw/widgets/ProgressDialog.h>

using namespace dsfw::widgets;

static QWidget *createPathSelectorDemo() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *group1 = new QGroupBox("OpenFile Mode");
    auto *l1 = new QVBoxLayout(group1);
    l1->addWidget(new PathSelector(PathSelector::OpenFile, "Audio File:", "WAV Files (*.wav);;All Files (*)"));
    layout->addWidget(group1);

    auto *group2 = new QGroupBox("SaveFile Mode");
    auto *l2 = new QVBoxLayout(group2);
    l2->addWidget(new PathSelector(PathSelector::SaveFile, "Output:", "TextGrid (*.TextGrid)"));
    layout->addWidget(group2);

    auto *group3 = new QGroupBox("Directory Mode");
    auto *l3 = new QVBoxLayout(group3);
    l3->addWidget(new PathSelector(PathSelector::Directory, "Working Dir:"));
    layout->addWidget(group3);

    layout->addStretch();
    return widget;
}

static QWidget *createProgressDemo() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *group1 = new QGroupBox("RunProgressRow");
    auto *l1 = new QVBoxLayout(group1);
    l1->addWidget(new RunProgressRow("Run Task"));
    layout->addWidget(group1);

    auto *group2 = new QGroupBox("FileProgressTracker (Label)");
    auto *l2 = new QVBoxLayout(group2);
    auto *tracker1 = new FileProgressTracker(FileProgressTracker::LabelOnly);
    tracker1->setProgress(42, 100);
    l2->addWidget(tracker1);
    layout->addWidget(group2);

    auto *group3 = new QGroupBox("FileProgressTracker (Bar)");
    auto *l3 = new QVBoxLayout(group3);
    auto *tracker2 = new FileProgressTracker(FileProgressTracker::ProgressBarStyle);
    tracker2->setProgress(75, 100);
    l3->addWidget(tracker2);
    layout->addWidget(group3);

    layout->addStretch();
    return widget;
}

static QWidget *createPropertyEditorDemo() {
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);

    auto *editor = new PropertyEditor;
    editor->addStringProperty("name", "Name:", "Test");
    editor->addIntProperty("samplerate", "Sample Rate:", 44100, 8000, 192000);
    editor->addDoubleProperty("threshold", "Threshold (dB):", -40.0, -100.0, 0.0, 1);
    editor->addBoolProperty("overwrite", "Overwrite existing:", false);
    editor->addChoiceProperty("format", "Output Format:", {"WAV", "FLAC", "OGG"}, 0);
    layout->addWidget(editor);

    return widget;
}

static QWidget *createLogViewerDemo() {
    auto *viewer = new LogViewer;
    viewer->appendLog(LogViewer::Debug, "Application started");
    viewer->appendLog(LogViewer::Info, "Loading model from /path/to/model.onnx");
    viewer->appendLog(LogViewer::Info, "Model loaded successfully");
    viewer->appendLog(LogViewer::Warning, "GPU device not found, falling back to CPU");
    viewer->appendLog(LogViewer::Error, "Failed to open audio file: permission denied");
    viewer->appendLog(LogViewer::Info, "Processing file 1 of 10");
    viewer->appendLog(LogViewer::Debug, "Feature extraction completed in 120ms");
    return viewer;
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Widget Gallery - dsfw-widgets Demo");
    window.resize(600, 500);

    auto *tabs = new QTabWidget;
    tabs->addTab(createPathSelectorDemo(), "PathSelector");
    tabs->addTab(createProgressDemo(), "Progress");
    tabs->addTab(createPropertyEditorDemo(), "PropertyEditor");
    tabs->addTab(createLogViewerDemo(), "LogViewer");

    window.setCentralWidget(tabs);
    window.show();

    return app.exec();
}
