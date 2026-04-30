#include <QTest>
#include <QSignalSpy>
#include <QThreadPool>
#include <dsfw/AsyncTask.h>

using namespace dstools;

class SuccessTask : public AsyncTask {
public:
    SuccessTask() : AsyncTask("success-task") {}
protected:
    bool execute(QString &msg) override {
        msg = "done";
        return true;
    }
};

class FailTask : public AsyncTask {
public:
    FailTask() : AsyncTask("fail-task") {}
protected:
    bool execute(QString &msg) override {
        msg = "something failed";
        return false;
    }
};

class TestAsyncTask : public QObject {
    Q_OBJECT
private slots:
    void testSuccessSignal();
    void testFailSignal();
    void testIdentifier();
};

void TestAsyncTask::testSuccessSignal() {
    auto *task = new SuccessTask();
    task->setAutoDelete(false);
    QSignalSpy spy(task, &AsyncTask::succeeded);
    QVERIFY(spy.isValid());

    QThreadPool::globalInstance()->start(task);
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("success-task"));
    QCOMPARE(spy.at(0).at(1).toString(), QString("done"));
    delete task;
}

void TestAsyncTask::testFailSignal() {
    auto *task = new FailTask();
    task->setAutoDelete(false);
    QSignalSpy spy(task, &AsyncTask::failed);
    QVERIFY(spy.isValid());

    QThreadPool::globalInstance()->start(task);
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("fail-task"));
    QCOMPARE(spy.at(0).at(1).toString(), QString("something failed"));
    delete task;
}

void TestAsyncTask::testIdentifier() {
    SuccessTask task;
    QCOMPARE(task.identifier(), QString("success-task"));
}

QTEST_GUILESS_MAIN(TestAsyncTask)
#include "TestAsyncTask.moc"
