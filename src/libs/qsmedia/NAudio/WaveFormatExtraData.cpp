#include "WaveFormatExtraData.h"

NAUDIO_BEGIN_NAMESPACE

WaveFormatExtraData::WaveFormatExtraData() {
    memset(extraData, 0, sizeof(extraData));
}

WaveFormatExtraData::WaveFormatExtraData(QIODevice *reader) : WaveFormat(reader) {
    this->ReadExtraData(reader);
}

void WaveFormatExtraData::ReadExtraData(QIODevice *reader) {
    if (this->extraSize <= (short) 0)
        return;
    reader->read(this->extraData, this->extraSize);
}

void WaveFormatExtraData::Serialize(QIODevice *writer) {
    WaveFormat::Serialize(writer);
    if (this->extraSize <= (short) 0)
        return;
    writer->write(this->extraData, this->extraSize);
}

NAUDIO_END_NAMESPACE