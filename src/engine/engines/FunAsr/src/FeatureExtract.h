
#ifndef FEATUREEXTRACT_H
#define FEATUREEXTRACT_H

#include <fftw3.h>

#include "FeatureQueue.h"
#include "SpeechWrap.h"
#include "Tensor.h"

namespace FunAsr {
    class FeatureExtract {
    private:
        SpeechWrap speech;
        FeatureQueue fqueue;
        int mode;
        bool m_valid = true;

        float *fft_input{};
        fftwf_complex *fft_out{};
        fftwf_plan p{};

        bool fftw_init();
        void melspect(const float *din, float *dout);
        void global_cmvn(float *din) const;

    public:
        explicit FeatureExtract(const int &mode);
        ~FeatureExtract();
        int size() const;
        void reset();
        void insert(float *din, int len, int flag);
        bool fetch(std::unique_ptr<Tensor<float>> &dout);
        [[nodiscard]] bool isValid() const { return m_valid; }
    };
}
#endif
