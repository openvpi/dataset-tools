/// @file WaveformRenderer.h
/// @brief Audio decoding and mono sample data management.

#pragma once

#include <QObject>
#include <QString>
#include <vector>

namespace dstools {
namespace phonemelabeler {

/// @brief Decodes audio files and provides mono sample data.
class WaveformRenderer : public QObject {
    Q_OBJECT

public:
    explicit WaveformRenderer(QObject *parent = nullptr);

    /// Decode the audio file at @p path into mono samples.
    /// @return true on success.
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
