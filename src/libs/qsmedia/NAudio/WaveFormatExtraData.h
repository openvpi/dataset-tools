#ifndef WAVEFORMATEXTRADATA_H
#define WAVEFORMATEXTRADATA_H

#include "WaveFormat.h"

#include "QsMediaGlobal.h"

NAUDIO_BEGIN_NAMESPACE

class QSMEDIA_API WaveFormatExtraData : public WaveFormat {
private:
    char extraData[100];

public:
    const char *ExtraData() const {
        return extraData;
    }

    WaveFormatExtraData();

    WaveFormatExtraData(QIODevice *reader);

    void ReadExtraData(QIODevice *reader);

    void Serialize(QIODevice *writer);
};

NAUDIO_END_NAMESPACE

#endif // WAVEFORMATEXTRADATA_H
