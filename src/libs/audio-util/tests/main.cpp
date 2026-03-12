#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <audio-util/Slicer.h>
#include <audio-util/Util.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <audio_in_path>  <wav_out_path>" << std::endl;
        return 1;
    }

    const std::filesystem::path audio_in_path = argv[1];
    const std::filesystem::path wav_out_path = argv[2];

    std::string errorMsg;
    auto sf_vio = AudioUtil::resample_to_vio(audio_in_path, errorMsg, 1, 44100);
    SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, sf_vio.info.channels,
                     sf_vio.info.samplerate);
    const auto totalSize = sf.frames();

    std::vector<float> audio(totalSize);
    sf.seek(0, SEEK_SET);
    sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));
    AudioUtil::write_vio_to_wav(sf_vio, wav_out_path, 1);

    return 0;
}
