#pragma once

#include <QObject>
#include <QString>
#include <vector>

namespace dstools {
namespace phonemelabeler {

class WaveformRenderer : public QObject {
    Q_OBJECT

public:
    explicit WaveformRenderer(QObject *parent = nullptr);

    bool loadAudio(const QString &path);

    [[nodiscard]] const std::vector<float> &samples() const { return m_samples; }
    [[nodiscard]] int sampleRate() const { return m_sampleRate; }
    [[nodiscard]] bool hasData() const { return !m_samples.empty(); }

signals:
    void audioLoaded(bool success);
    void error(const QString &msg);

private:
    std::vector<float> m_samples;
    int m_sampleRate = 0;
};

} // namespace phonemelabeler
} // namespace dstools