#include <QTest>

// Include directly to ensure static registration is linked
#include "SlicerProcessor.h"

#include <dsfw/TaskProcessorRegistry.h>

using namespace dstools;

class TestSlicerProcessor : public QObject {
    Q_OBJECT
private slots:
    void direct_create();
    void task_spec();
};

void TestSlicerProcessor::direct_create() {
    SlicerProcessor proc;
    QCOMPARE(proc.processorId(), QStringLiteral("slicer"));
    QCOMPARE(proc.displayName(), QStringLiteral("Audio Slicer"));
}

void TestSlicerProcessor::task_spec() {
    SlicerProcessor proc;
    auto spec = proc.taskSpec();
    QCOMPARE(spec.taskName, QStringLiteral("audio_slice"));
    QCOMPARE(spec.outputs.size(), size_t(1));
    QCOMPARE(spec.outputs[0].category, QStringLiteral("slices"));
}

QTEST_MAIN(TestSlicerProcessor)
#include "TestSlicerProcessor.moc"
