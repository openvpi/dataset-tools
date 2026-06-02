#include <QtTest/QtTest>

#include <QLabel>
#include <QApplication>
#include <QSettings>

#include <dsfw/widgets/ViewportController.h>
#include <dsfw/AppSettings.h>

#include "audio-visualizer/AudioVisualizerContainer.h"

using namespace dstools;
using namespace dsfw::widgets;

class TestAudioVisualizerContainer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void construction();
    void viewport_access();
    void set_total_duration();
    void viewport_zoom();
    void viewport_scroll();
    void add_chart();
    void remove_chart();
    void chart_order();
    void chart_visibility();
    void resolution_persistence();
    void fit_to_window();
    void set_audio_data();
    void set_boundary_model();
};

void TestAudioVisualizerContainer::initTestCase() {
    QCoreApplication::setOrganizationName(QStringLiteral("dataset-tools-test"));
    QCoreApplication::setApplicationName(QStringLiteral("TestAudioVisualizerContainer"));
}

void TestAudioVisualizerContainer::cleanupTestCase() {
    QSettings settings;
    settings.clear();
}

void TestAudioVisualizerContainer::construction() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    QVERIFY(container.viewport() != nullptr);
    QVERIFY(container.timeRuler() != nullptr);
    QVERIFY(container.boundaryOverlay() != nullptr);
    QVERIFY(container.chartSplitter() != nullptr);
    QVERIFY(container.miniMap() != nullptr);
    QVERIFY(container.tierLabelArea() != nullptr);
    QVERIFY(container.dragController() != nullptr);
}

void TestAudioVisualizerContainer::viewport_access() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    auto* vp = container.viewport();
    QVERIFY(vp != nullptr);
    QCOMPARE(vp->startSec(), 0.0);
    QCOMPARE(vp->endSec(), 10.0);
    QCOMPARE(vp->totalDuration(), 0.0);
}

void TestAudioVisualizerContainer::set_total_duration() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    container.setTotalDuration(120.0);
    auto* vp = container.viewport();
    QCOMPARE(vp->totalDuration(), 120.0);
}

void TestAudioVisualizerContainer::viewport_zoom() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    container.setTotalDuration(60.0);
    auto* vp = container.viewport();
    vp->setViewRange(0.0, 10.0);
    int oldRes = vp->resolution();
    vp->zoomIn(5.0);
    QVERIFY(vp->resolution() < oldRes);
    vp->zoomOut(5.0);
    QVERIFY(vp->resolution() > vp->resolution() || vp->resolution() == oldRes);
}

void TestAudioVisualizerContainer::viewport_scroll() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    container.setTotalDuration(60.0);
    auto* vp = container.viewport();
    vp->setViewRange(5.0, 15.0);
    vp->scrollBy(5.0);
    QCOMPARE(vp->startSec(), 10.0);
    QCOMPARE(vp->endSec(), 20.0);
}

void TestAudioVisualizerContainer::add_chart() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    auto* label = new QLabel(QStringLiteral("test chart"));
    container.addChart(QStringLiteral("chart1"), label, 0);
    auto order = container.chartOrder();
    QVERIFY(order.contains(QStringLiteral("chart1")));
}

void TestAudioVisualizerContainer::remove_chart() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    auto* label = new QLabel(QStringLiteral("test chart"));
    container.addChart(QStringLiteral("chart1"), label, 0);
    container.removeChart(QStringLiteral("chart1"));
    auto order = container.chartOrder();
    QVERIFY(!order.contains(QStringLiteral("chart1")));
}

void TestAudioVisualizerContainer::chart_order() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    auto* label1 = new QLabel(QStringLiteral("c1"));
    auto* label2 = new QLabel(QStringLiteral("c2"));
    auto* label3 = new QLabel(QStringLiteral("c3"));
    container.addChart(QStringLiteral("c1"), label1, 0);
    container.addChart(QStringLiteral("c2"), label2, 1);
    container.addChart(QStringLiteral("c3"), label3, 2);

    container.setChartOrder({QStringLiteral("c3"), QStringLiteral("c1"), QStringLiteral("c2")});
    auto order = container.chartOrder();
    QCOMPARE(order.size(), 3);
    QCOMPARE(order[0], QStringLiteral("c3"));
    QCOMPARE(order[1], QStringLiteral("c1"));
    QCOMPARE(order[2], QStringLiteral("c2"));
}

void TestAudioVisualizerContainer::chart_visibility() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    auto* label = new QLabel(QStringLiteral("test chart"));
    container.addChart(QStringLiteral("chart1"), label, 0);
    container.setChartVisible(QStringLiteral("chart1"), false);
    QVERIFY(!container.isChartVisible(QStringLiteral("chart1")));
    container.setChartVisible(QStringLiteral("chart1"), true);
    QVERIFY(container.isChartVisible(QStringLiteral("chart1")));
}

void TestAudioVisualizerContainer::resolution_persistence() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    container.setTotalDuration(60.0);
    container.setResolutionKey(QStringLiteral("Viewport/test/resolution"));

    auto* vp = container.viewport();
    vp->setResolution(200);
    container.saveResolution();

    QSettings settings;
    QVERIFY(settings.value(QStringLiteral("Viewport/test/resolution")).toInt() == 200);

    AudioVisualizerContainer container2(QStringLiteral("test"), nullptr);
    container2.setTotalDuration(60.0);
    container2.setResolutionKey(QStringLiteral("Viewport/test/resolution"));
    QVERIFY(container2.restoreResolution());
    QCOMPARE(container2.viewport()->resolution(), 200);
}

void TestAudioVisualizerContainer::fit_to_window() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    container.setTotalDuration(30.0);
    container.fitToWindow();
    QVERIFY(container.viewport()->resolution() > 0);
}

void TestAudioVisualizerContainer::set_audio_data() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    std::vector<float> samples(44100, 0.0f);
    container.setAudioData(samples, 44100);
    QVERIFY(container.miniMap() != nullptr);
}

void TestAudioVisualizerContainer::set_boundary_model() {
    AudioVisualizerContainer container(QStringLiteral("test"), nullptr);
    QVERIFY(container.boundaryModel() == nullptr);
    container.setBoundaryModel(nullptr);
    QVERIFY(container.boundaryModel() == nullptr);
}

QTEST_MAIN(TestAudioVisualizerContainer)
#include "TestAudioVisualizerContainer.moc"