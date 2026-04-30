#include "WorkThread.h"
#include "SliceJob.h"

WorkThread::WorkThread(const QString &filename, const QString &outPath, double threshold, qint64 minLength,
                       qint64 minInterval, qint64 hopSize, qint64 maxSilKept, int outputWaveFormat, bool saveAudio,
                       bool saveMarkers, bool loadMarkers, bool overwriteMarkers, int minimumDigits)
    : AsyncTask(filename), m_outPath(outPath), m_threshold(threshold), m_minLength(minLength),
      m_minInterval(minInterval), m_hopSize(hopSize), m_maxSilKept(maxSilKept), m_outputWaveFormat(outputWaveFormat),
      m_saveAudio(saveAudio), m_saveMarkers(saveMarkers), m_loadMarkers(loadMarkers),
      m_overwriteMarkers(overwriteMarkers), m_minimumDigits(minimumDigits) {
}

bool WorkThread::execute(QString &msg) {
    struct SinkAdapter : ISliceJobSink {
        WorkThread *self;
        explicit SinkAdapter(WorkThread *s) : self(s) {}
        void onInfo(const QString &m) override { emit self->oneInfo(m); }
        void onError(const QString &m) override { emit self->oneError(m); }
    } sink(this);

    SliceJobParams params{};
    params.threshold = m_threshold;
    params.minLength = m_minLength;
    params.minInterval = m_minInterval;
    params.hopSize = m_hopSize;
    params.maxSilKept = m_maxSilKept;
    params.outputWaveFormat = m_outputWaveFormat;
    params.saveAudio = m_saveAudio;
    params.saveMarkers = m_saveMarkers;
    params.loadMarkers = m_loadMarkers;
    params.overwriteMarkers = m_overwriteMarkers;
    params.minimumDigits = m_minimumDigits;

    auto result = SliceJob::run(identifier(), m_outPath, params, &sink);
    msg = result.errorMessage;
    return result.success;
}
