#pragma once 
// system
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <deque>
#include <iostream>
#include <list>
#include <clocale>
#include <vector>
#include <string>
#include <cmath>
#include <numeric>
#include <cstring>

using namespace std;

// third part
#include <fftw3.h>
#include "onnxruntime_run_options_config_keys.h"
#include "onnxruntime_cxx_api.h"

// mine
#include "commonfunc.h"
#include <ComDefine.h>
#include "predefine_coe.h"

#include <ComDefine.h>
#include "Vocab.h"
#include "Tensor.h"
#include "util.h"
#include "CommonStruct.h"
#include "FeatureExtract.h"
#include "FeatureQueue.h"
#include "SpeechWrap.h"
#include <Audio.h>
#include "Model.h"
#include "paraformer_onnx.h"
#include "librapidasrapi.h"

using namespace paraformer;
