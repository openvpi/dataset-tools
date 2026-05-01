#ifndef RMVPEMODEL_H
#define RMVPEMODEL_H

#include <filesystem>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <rmvpe-infer/RmvpeGlobal.h>
#include <rmvpe-infer/Provider.h>
#include <string>
#include <vector>

#include <dstools/OnnxModelBase.h>

namespace Rmvpe
{
    class RMVPE_INFER_EXPORT RmvpeModel : public dstools::infer::CancellableOnnxModel {
    public:
        explicit RmvpeModel(const std::filesystem::path &modelPath, ExecutionProvider provider, int device_id);
        ~RmvpeModel();

        bool is_open() const { return isOpen(); }
        bool forward(const std::vector<float> &waveform_data, float threshold, std::vector<float> &f0,
                     std::vector<bool> &uv, std::string &msg);

    private:
        Ort::AllocatorWithDefaultOptions m_allocator;
        const char *m_waveform_input_name;
        const char *m_threshold_input_name;
        const char *m_f0_output_name;
        const char *m_uv_output_name;
    };

} // namespace Rmvpe
#endif // RMVPEMODEL_H
