#include "FblModel.h"

#include <syscmdline/system.h>

namespace FBL {
    FblModel::FblModel(const std::string &model_path)
        : m_env(Ort::Env(ORT_LOGGING_LEVEL_WARNING, "FblModel")), m_session_options(Ort::SessionOptions()),
          m_session(nullptr) {

        m_input_name = "mel";
        m_output_name = "ap_probability";

#ifdef _WIN32
        const std::wstring wstrPath = SysCmdLine::utf8ToWide(model_path);
        m_session = new Ort::Session(m_env, wstrPath.c_str(), m_session_options);
#else
        m_session = new Ort::Session(m_env, model_path.c_str(), m_session_options);
#endif
    }

    FblModel::~FblModel() {
        delete m_session;
        m_input_name = {};
        m_output_name = {};
    }

    bool FblModel::forward(const std::vector<std::vector<float>> &input_data, std::vector<float> &result,
                           std::string &msg) const {
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
            result = std::vector<float>(
                float_array, float_array + output_tensors.front().GetTensorTypeAndShapeInfo().GetElementCount());
            return true;
        } catch (const Ort::Exception &e) {
            msg = "Error during model inference: " + std::string(e.what());
            return false;
        }
    }
} // FBL