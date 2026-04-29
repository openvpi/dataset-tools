#include <array>
#include <iostream>

#include <dstools/OnnxEnv.h>
#include <some-infer/SomeModel.h>

namespace Some
{
    SomeModel::SomeModel(const std::filesystem::path &modelPath, const ExecutionProvider provider, const int device_id) :
        m_session_options(dstools::infer::OnnxEnv::createSessionOptions(provider, device_id)),
        m_session(nullptr), m_waveform_input_name("waveform"), m_note_midi_output_name("note_midi"),
        m_note_rest_output_name("note_rest"), m_note_dur_output_name("note_dur") {

        // Create ONNX Runtime Session
        try {
#ifdef _WIN32
            m_session = Ort::Session(dstools::infer::OnnxEnv::env(), modelPath.wstring().c_str(), m_session_options);
#else
            m_session = Ort::Session(dstools::infer::OnnxEnv::env(), modelPath.c_str(), m_session_options);
#endif
        } catch (const Ort::Exception &e) {
            std::cout << "Failed to create session: " << e.what() << std::endl;
        }
    }

    // Destructor: Release ONNX session
    SomeModel::~SomeModel() = default;

    void SomeModel::terminate() {
        std::lock_guard lock(m_runMutex);
        if (m_activeRunOptions) m_activeRunOptions->SetTerminate();
    }

    bool SomeModel::is_open() const { 
        return m_session != nullptr;
    }

    // Forward pass through the model: takes waveform and threshold as inputs, returns f0 and uv as outputs
    bool SomeModel::forward(const std::vector<float> &waveform_data, std::vector<float> &note_midi,
                            std::vector<bool> &note_rest, std::vector<float> &note_dur, std::string &msg) {
        if (!m_session) {
            msg = "Session is not initialized.";
            return false;
        }
        try {
            const size_t n_samples = waveform_data.size();

            const std::array<int64_t, 2> input_waveform_shape = {1, static_cast<int64_t>(n_samples)};
            std::vector<float> waveform_mutable(waveform_data.begin(), waveform_data.end());
            Ort::Value waveform_tensor = Ort::Value::CreateTensor<float>(
                m_memory_info, waveform_mutable.data(), waveform_mutable.size(),
                input_waveform_shape.data(), input_waveform_shape.size());

            std::vector<float> midi_data;
            std::vector<int> rest_data;
            std::vector<int> dur_data;

            const char *input_names[] = {"waveform"};
            const char *output_names[] = {"note_midi", "note_rest", "note_dur"};

            const Ort::Value input_tensors[] = {std::move(waveform_tensor)};

            Ort::RunOptions runOptions;
            {
                std::lock_guard lock(m_runMutex);
                m_activeRunOptions = &runOptions;
            }
            auto output_tensors = m_session.Run(runOptions, input_names, input_tensors, 1, output_names, 3);
            {
                std::lock_guard lock(m_runMutex);
                m_activeRunOptions = nullptr;
            }

            const float *midi_array = output_tensors.at(0).GetTensorMutableData<float>();
            note_midi.assign(midi_array,
                             midi_array + output_tensors.at(0).GetTensorTypeAndShapeInfo().GetElementCount());

            const bool *uv_array = output_tensors.at(1).GetTensorMutableData<bool>();
            note_rest.assign(uv_array, uv_array + output_tensors.at(1).GetTensorTypeAndShapeInfo().GetElementCount());

            const float *dur_array = output_tensors.at(2).GetTensorMutableData<float>();
            note_dur.assign(dur_array, dur_array + output_tensors.at(2).GetTensorTypeAndShapeInfo().GetElementCount());

            return true;
        }
        catch (const Ort::Exception &e) {
            msg = "Error during model inference: " + std::string(e.what());
            return false;
        }
    }

} // namespace Some
