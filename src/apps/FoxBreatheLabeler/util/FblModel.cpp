#include "FblModel.h"

#include <syscmdline/system.h>

namespace FBL {
    FblModel::FblModel(const std::string &model_path)
        : m_env(Ort::Env(ORT_LOGGING_LEVEL_WARNING, "FblModel")), m_session_options(Ort::SessionOptions()),
          m_session(nullptr) {

        m_input_name = "waveform";
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
        const size_t batch_size = input_data.size();
        if (batch_size == 0) {
            throw std::invalid_argument("输入数据不能为空。");
        }

        // 确定输入数据中最大的长度
        size_t max_height = 0;
        for (const auto &channel_data : input_data) {
            max_height = std::max(max_height, channel_data.size());
        }

        // 创建一个用于存放扁平化后的输入数据的向量
        std::vector<float> flattened_input(batch_size * max_height, 0.0f);

        // 将输入数据扁平化并填充到flattened_input中
        for (size_t i = 0; i < batch_size; ++i) {
            std::copy(input_data[i].begin(), input_data[i].end(), flattened_input.begin() + i * max_height);
        }

        // 定义输入张量的形状
        const std::array<int64_t, 2> input_shape_{static_cast<int64_t>(batch_size), static_cast<int64_t>(max_height)};

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