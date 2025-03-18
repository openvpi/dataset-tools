#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include <audio-util/Util.h>


int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <audio_in_path>  <wav_out_path>" << std::endl;
        return 1;
    }

    const std::filesystem::path audio_in_path = argv[1];
    const std::filesystem::path wav_out_path = argv[2];

    std::string errorMsg;
    auto sf_vio = AudioUtil::resample_to_vio(audio_in_path, errorMsg, 1, 16000);
    AudioUtil::write_vio_to_wav(sf_vio, wav_out_path, 1);

    return 0;
}
