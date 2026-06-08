#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include <QThreadPool>
#include <dsfw/AsyncTask.h>

using namespace std::chrono_literals;

class SuccessTask : public dsfw::AsyncTask {
public:
    SuccessTask() : dsfw::AsyncTask("success-task") {}
protected:
    bool execute(QString &msg) override {
        msg = "done";
        return true;
    }
};

class FailTask : public dsfw::AsyncTask {
public:
    FailTask() : dsfw::AsyncTask("fail-task") {}
protected:
    bool execute(QString &msg) override {
        msg = "something failed";
        return false;
    }
};

class CancellableTask : public dsfw::AsyncTask {
public:
    CancellableTask() : dsfw::AsyncTask("cancel-task") {}
protected:
    bool execute(QString &msg) override {
        for (int i = 0; i < 100 && !isCanceled(); ++i)
            QThread::msleep(10);
        if (isCanceled()) {
            msg = "canceled";
            return false;
        }
        msg = "done";
        return true;
    }
};

class TestAsyncTask : public QObject {
    Q_OBJECT
private slots:
    void testSuccessSignal();
    void testFailSignal();
    void testIdentifier();
    void testTimeout_default();
    void testCancel();
    void testHasTimedOut();
};

void TestAsyncTask::testSuccessSignal() {
    auto *task = new SuccessTask();
    QSignalSpy spy(task, &AsyncTask::succeeded);
    QVERIFY(spy.isValid());

    QThreadPool::globalInstance()->start(task);
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("success-task"));
    QCOMPARE(spy.at(0).at(1).toString(), QString("done"));
    // task deletes itself via deleteLater() after run()
}

void TestAsyncTask::testFailSignal() {
    auto *task = new FailTask();
    QSignalSpy spy(task, &AsyncTask::failed);
    QVERIFY(spy.isValid());

    QThreadPool::globalInstance()->start(task);
    QVERIFY(spy.wait(5000));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("fail-task"));
    QCOMPARE(spy.at(0).at(1).toString(), QString("something failed"));
    // task deletes itself via deleteLater() after run()
}

void TestAsyncTask::testIdentifier() {
    SuccessTask task;
    QCOMPARE(task.identifier(), QString("success-task"));
}

void TestAsyncTask::testTimeout_default() {
    SuccessTask task;
    QCOMPARE(task.timeout(), std::chrono::milliseconds(30000));
}

void TestAsyncTask::testCancel() {
    auto *task = new CancellableTask();
    QSignalSpy spy(task, &AsyncTask::failed);
    QVERIFY(spy.isValid());

    QThreadPool::globalInstance()->start(task);
    QThread::msleep(50);
    task->requestCancel();
    QVERIFY(spy.wait(5000));
    QVERIFY(task->isCanceled());
    QVERIFY(task->isFinished());
    QCOMPARE(spy.at(0).at(1).toString(), QString("canceled"));
}

void TestAsyncTask::testHasTimedOut() {
    SuccessTask task;
    task.setTimeout(0ms);
    QVERIFY(!task.hasTimedOut());

    task.setTimeout(1ms);
    QThread::msleep(10);
    QVERIFY(task.hasTimedOut());
}

QTEST_GUILESS_MAIN(TestAsyncTask)
#include "TestAsyncTask.moc"
