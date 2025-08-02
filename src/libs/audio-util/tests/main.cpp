#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <audio-util/Util.h>
#include <audio-util/Slicer.h>


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
    //AudioUtil::write_vio_to_wav(sf_vio, wav_out_path, 1);

    AudioUtil::Slicer slicer(44100, 0.02f, 441, 1764, 500, 30, 50);
    auto chunks = slicer.slice(audio);

    std::ostringstream markerCsv;
    markerCsv << "Name\tStart\tDuration\tTime Format\tType\tDescription\n";
    int idx = 0;
    for (auto chunk : chunks) {
        auto beginFrame = chunk.first;
        auto endFrame = chunk.second;
        auto frameCount = endFrame - beginFrame;
        if ((frameCount <= 0) || (beginFrame > totalSize) || (endFrame > totalSize) || (beginFrame < 0) ||
            (endFrame < 0)) {
            continue;
            }

        markerCsv << "Marker " << idx << '\t' << beginFrame << '\t' << endFrame - beginFrame << '\t' << "44100 Hz" << '\t' << "Cue" << '\t' << '\n';
        std::ostringstream oss;
        oss << "output_" << idx << ".wav";

        SndfileHandle wf = SndfileHandle(oss.str(), SFM_WRITE,
                                         SF_FORMAT_WAV | SF_FORMAT_FLOAT, 1, 44100);
        sf.seek(beginFrame, SEEK_SET);
        auto bytesWritten = wf.write(audio.data() + beginFrame, endFrame - beginFrame);
        if (bytesWritten == 0) {
            std::cout << sf.strError() << '\n';
            return 1;
        }
        idx++;
    }

    std::ofstream markerFile("output_markers.csv");
    markerFile << markerCsv.str();
    return 0;
}
