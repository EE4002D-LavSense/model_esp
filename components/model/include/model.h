#include "dl_model_base.hpp"
#include "esp_log.h"
#include "fbs_loader.hpp"
#include <type_traits>

#include "sd_card.h"
#include "driver/i2s_std.h"
#include "spectrogram.h"    

#define MODEL_TAG "MODEL"

using namespace dl;

extern "C" void modelInference(Model *model, i2s_chan_handle_t rx_handle, bool *doorClose, bool *waterFlow);

// Function to print the contents of a TensorBase using ESP_LOGI
extern "C" void log_tensor_data(const std::string& tensor_name, dl::TensorBase* tensor);

// Function to iterate over model outputs and log them
extern "C" void log_model_outputs(std::map<std::string, TensorBase*> model_outputs);