#include "WorkThread.h"
#include "SliceJob.h"

WorkThread::WorkThread(const QString &filename, const QString &outPath, const SliceJobParams &params)
    : AsyncTask(filename), m_outPath(outPath), m_params(params) {
}

bool WorkThread::execute(QString &msg) {
    struct SinkAdapter : ISliceJobSink {
        WorkThread *self;
        explicit SinkAdapter(WorkThread *s) : self(s) {}
        void onInfo(const QString &m) override { emit self->oneInfo(m); }
        void onError(const QString &m) override { emit self->oneError(m); }
    } sink(this);

    auto result = SliceJob::run(identifier(), m_outPath, m_params, &sink);
    msg = result.errorMessage;
    return result.success;
}
