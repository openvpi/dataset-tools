#pragma once

#include <QString>
#include <utility>
#include <vector>

struct AudioData;

using MarkerList = std::vector<std::pair<qint64, qint64>>;

struct ChunkWriteResult {
    bool success = false;
    int chunksWritten = 0;
};

class AudioChunkWriter {
public:
    static ChunkWriteResult writeChunks(const AudioData &audio, const MarkerList &chunks,
                                        const QString &outputDir, const QString &fileBaseName,
                                        int outputWaveFormat, int minimumDigits);
};
