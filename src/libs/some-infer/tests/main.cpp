#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <some-infer/Some.h>

#include "wolf-midi/MidiFile.h"

static void progressChanged(const int progress) { std::cout << "progress: " << progress << "%" << std::endl; }

int main(int argc, char *argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: " << argv[0]
                  << " <model_path> <wav_path> <dml/cpu> <device_id> <out_midi_path> <tempo:float>" << std::endl;
        return 1;
    }

    const std::filesystem::path modelPath = argv[1];
    const std::filesystem::path wavPath = argv[2];
    const std::string provider = argv[3];
    const int device_id = std::stoi(argv[4]);
    const std::filesystem::path outMidiPath = argv[5];
    const float tempo = std::stof(argv[6]);

    const auto someProvider = provider == "dml" ? Some::ExecutionProvider::DML : Some::ExecutionProvider::CPU;

    const Some::Some some(modelPath, someProvider, device_id);

    std::vector<Some::Midi> midis;
    std::string msg;

    bool success = some.get_midi(wavPath, midis, tempo, msg, progressChanged);

    if (success) {
        Midi::MidiFile midi;
        midi.setFileFormat(1);
        midi.setDivisionType(Midi::MidiFile::DivisionType::PPQ);
        midi.setResolution(480);

        midi.createTrack();

        midi.createTimeSignatureEvent(0, 0, 4, 4);
        midi.createTempoEvent(0, 0, tempo);

        std::vector<char> trackName;
        std::string str = "Some";
        trackName.insert(trackName.end(), str.begin(), str.end());

        midi.createTrack();
        midi.createMetaEvent(1, 0, Midi::MidiEvent::MetaNumbers::TrackName, trackName);

        for (const auto &[note, start, duration] : midis) {
            midi.createNoteOnEvent(1, start, 0, note, 64);
            midi.createNoteOffEvent(1, start + duration, 0, note, 64);
        }

        midi.save(outMidiPath);
    } else {
        std::cerr << "Error: " << msg << std::endl;
    }

    return 0;
}
