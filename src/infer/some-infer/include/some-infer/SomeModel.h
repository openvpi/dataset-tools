#ifndef SOMEMODEL_H
#define SOMEMODEL_H

#include <filesystem>
#include <memory>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <some-infer/Provider.h>
#include <string>
#include <vector>

namespace Some
{
    class SomeModel {
    public:
        explicit SomeModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~SomeModel();

        bool is_open() const;
        bool forward(const std::vector<float> &waveform_data, std::vector<float> &note_midi,
                     std::vector<bool> &note_rest, std::vector<float> &note_dur, std::string &msg);

        void terminate();

    private:
        std::mutex m_runMutex;
        Ort::RunOptions *m_activeRunOptions = nullptr;
        std::unique_ptr<Ort::Session> m_session;
        const char *m_waveform_input_name;
        const char *m_note_midi_output_name;
        const char *m_note_rest_output_name;
        const char *m_note_dur_output_name;

#ifdef _WIN_X86
        Ort::MemoryInfo m_memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
#else
        Ort::MemoryInfo m_memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
#endif
    };

} // namespace Some
#endif // SOMEMODEL_H
