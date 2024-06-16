#include "FblModel.h"

#include <syscmdline/system.h>

namespace FBL {
    FblModel::FblModel(const std::string &model_path)
        : m_env(std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FblModel")),
          m_session_options(std::make_unique<Ort::SessionOptions>()) {

        m_input_name = "mel";
        m_output_name = "ap_probability";

#ifdef _WIN32
        const std::wstring wstrPath = SysCmdLine::utf8ToWide(model_path);
        m_session = std::make_unique<Ort::Session>(*m_env, wstrPath.c_str(), *m_session_options);
#else
        m_session = new Ort::Session(env, model_path.c_str(), sessionOptions);
#endif

        m_input_dims = m_session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
        m_output_dims = m_session->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
    }

    std::vector<float> FblModel::forward(const std::vector<std::vector<float>> &input_data) const {
        // 假设输入数据是二维的，形状为 (num_channels, height)
        const size_t num_channels = input_data.size();
        if (num_channels == 0) {
            throw std::invalid_argument("Input data cannot be empty.");
        }

        const size_t height = input_data[0].size();

        // 将输入数据展平成一维数组
        std::vector<float> flattened_input;
        for (const auto &channel_data : input_data) {
            flattened_input.insert(flattened_input.end(), channel_data.begin(), channel_data.end());
        }

        // 定义输入张量的形状，batch size 固定为 1
        const std::array<int64_t, 3> input_shape_{1, static_cast<int64_t>(num_channels), static_cast<int64_t>(height)};

        // 创建输入张量
        const Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            m_memoryInfo, flattened_input.data(), flattened_input.size(), input_shape_.data(), input_shape_.size());

        try {
            auto output_tensors =
                m_session->Run(Ort::RunOptions{nullptr}, &m_input_name, &input_tensor, 1, &m_output_name, 1);

            const float *float_array = output_tensors.front().GetTensorMutableData<float>();
            std::vector<float> results(
                float_array, float_array + output_tensors.front().GetTensorTypeAndShapeInfo().GetElementCount());

            // validate_output(results);

            return results;
        } catch (const Ort::Exception &e) {
            throw std::runtime_error("Error during model inference: " + std::string(e.what()));
        }
    }

    void FblModel::validate_input(const std::vector<float> &input_data) const {
        size_t expected_size = 1;
        for (const auto dim : m_input_dims) {
            expected_size *= dim;
        }
        if (input_data.size() != expected_size) {
            throw std::invalid_argument("Input data size does not match the model's expected input size.");
        }
    }

    void FblModel::validate_output(const std::vector<float> &output_data) const {
        size_t expected_size = 1;
        for (const auto dim : m_output_dims) {
            expected_size *= dim;
        }
        if (output_data.size() != expected_size) {
            throw std::runtime_error("Output data size does not match the model's expected output size.");
        }
    }
} // FBL