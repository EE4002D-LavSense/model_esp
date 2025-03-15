#include "dl_model_base.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "fbs_loader.hpp"
#include <type_traits>
#include "esp_heap_caps.h"
#include "driver/i2s_std.h"

#include "mic.h"
#include "sd_card.h"
#include "spectrogram.h"

static const char *TAG = "APP MAIN";

// using namespace fbs;
using namespace dl;

// *****************************
// I2S 
// *****************************
i2s_chan_handle_t rx_handle;


// *****************************
// ESP DL Functions
// *****************************
template <typename ground_truth_element_t>
void compare_elementwise(const ground_truth_element_t *ground_truth, TensorBase *infer_value)
{
    if (!ground_truth || !infer_value || !infer_value->get_element_ptr()) {
        ESP_LOGE(TAG,
                 "empty data, ground_truth: %p, infer_value: %p, infer_value->get_element_ptr(): %p",
                 ground_truth,
                 infer_value,
                 infer_value->get_element_ptr());
        return;
    }

    ESP_LOGI(TAG, "output size: %d", infer_value->get_size());
    ground_truth_element_t *infer_value_pointer = static_cast<ground_truth_element_t *>(infer_value->get_element_ptr());

    for (int i = 0; i < infer_value->get_size(); i++) {
        if (ground_truth[i] != infer_value_pointer[i]) {
            ESP_LOGE(TAG,
                     "Inconsistent values, ground true: %ld, infer: %ld",
                     static_cast<int32_t>(ground_truth[i]),
                     static_cast<int32_t>(infer_value_pointer[i]));
            std::vector<int> value_position = infer_value->get_axis_index(i);
            ESP_LOGE(TAG, "The position of inconsistent values: %s", dl::shape_to_string(value_position).c_str());
        }
    }
}

void compare_test_outputs(Model *model, std::map<std::string, TensorBase *> infer_outputs)
{
    if (!model) {
        return;
    }

    fbs::FbsModel *fbs_model_instance = model->get_fbs_model();
    fbs_model_instance->load_map();
    for (auto infer_outputs_iter = infer_outputs.begin(); infer_outputs_iter != infer_outputs.end();
         infer_outputs_iter++) {
        std::string infer_output_name = infer_outputs_iter->first;
        const void *ground_truth_data = fbs_model_instance->get_test_output_tensor_raw_data(infer_output_name);
        if (!ground_truth_data) {
            ESP_LOGE(TAG, "The infer output(%s) isn't found in model's ground truth.", infer_output_name.c_str());
            return;
        }
        TensorBase *infer_output = infer_outputs_iter->second;
        ESP_LOGI(TAG,
                 "infer_output, name: %s, shape: %s",
                 infer_outputs_iter->first.c_str(),
                 dl::shape_to_string(infer_output->get_shape()).c_str());

        if (infer_output->dtype == dl::DATA_TYPE_INT8) {
            compare_elementwise(static_cast<const int8_t *>(ground_truth_data), infer_output);
        } else if (infer_output->dtype == dl::DATA_TYPE_INT16) {
            compare_elementwise(static_cast<const int16_t *>(ground_truth_data), infer_output);
        } else if (infer_output->dtype == dl::DATA_TYPE_FLOAT) {
        }
    }

    return;
}

std::map<std::string, TensorBase *> get_graph_test_inputs(Model *model)
{
    std::map<std::string, TensorBase *> test_inputs;

    if (!model) {
        return test_inputs;
    }

    fbs::FbsModel *parser_instance = model->get_fbs_model();
    parser_instance->load_map();
    std::map<std::string, TensorBase *> graph_inputs = model->get_inputs();
    for (auto graph_inputs_iter = graph_inputs.begin(); graph_inputs_iter != graph_inputs.end(); graph_inputs_iter++) {
        std::string input_name = graph_inputs_iter->first;
        TensorBase *input = graph_inputs_iter->second;

        if (input) {
            const void *input_data = parser_instance->get_test_input_tensor_raw_data(input_name);
            if (input_data) {
                TensorBase *test_input =
                    new TensorBase(input->shape, input_data, input->exponent, input->dtype, false, MALLOC_CAP_SPIRAM);
                test_inputs.emplace(input_name, test_input);
            }
        }
    }

    return test_inputs;
}

// ********************************
// My test
// ********************************

static const char* MODELTAG = "MODEL_OUTPUT_PRINTER";

// Function to print the contents of a TensorBase using ESP_LOGI
void log_tensor_data(const std::string& tensor_name, dl::TensorBase* tensor) {
    if (!tensor) {
        ESP_LOGE(MODELTAG, "Tensor %s is null and cannot be printed.", tensor_name.c_str());
        return;
    }

    std::vector<int> shape = tensor->get_shape();
    std::string shape_str;
    for (int dim : shape) {
        shape_str += std::to_string(dim) + " ";
    }
    //ESP_LOGI(MODELTAG, "Tensor name: %s, shape: [%s]", tensor_name.c_str(), shape_str.c_str());

    size_t num_elements = 1;
    for (int dim : shape) {
        num_elements *= dim;
    }

    if (tensor->dtype == dl::DATA_TYPE_INT8) {
        const int8_t* data = static_cast<const int8_t*>(tensor->data);
        std::string data_str;
        for (size_t i = 0; i < num_elements; ++i) {
            data_str += std::to_string(static_cast<int>(data[i])) + " ";
        }
        ESP_LOGI(MODELTAG, "Data (int8): %s", data_str.c_str());
    } else if (tensor->dtype == dl::DATA_TYPE_INT16) {
        const int16_t* data = static_cast<const int16_t*>(tensor->data);
        std::string data_str;
        for (size_t i = 0; i < num_elements; ++i) {
            data_str += std::to_string(data[i]) + " ";
        }
        ESP_LOGI(MODELTAG, "Data (int16): %s", data_str.c_str());
    } else if (tensor->dtype == dl::DATA_TYPE_FLOAT) {
        const float* data = static_cast<const float*>(tensor->data);
        std::string data_str;
        for (size_t i = 0; i < num_elements; ++i) {
            data_str += std::to_string(data[i]) + " ";
        }
        ESP_LOGI(MODELTAG, "Data (float): %s", data_str.c_str());

        // Find argmax
        size_t max_index = 0;
        float max_value = data[0];
        
        // Find the index of the maximum value
        for (size_t i = 1; i < num_elements; ++i) {
            if (data[i] > max_value) {
                max_value = data[i];
                max_index = i;
            }
        }
        ESP_LOGI(MODELTAG, "Class: %d, Max value: %f", max_index, max_value);
    } else {
        ESP_LOGE(MODELTAG, "Unsupported data type for tensor %s.", tensor_name.c_str());
    }
}

// Function to iterate over model outputs and log them
void log_model_outputs(std::map<std::string, TensorBase*> model_outputs) {
    for (const auto& output_pair : model_outputs) {
        const std::string& output_name = output_pair.first;
        TensorBase* output_tensor = output_pair.second;
        log_tensor_data(output_name, output_tensor);
    }
}


// *****************************
// Main
// *****************************

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "get into app_main");
    printf("heap_caps_get_free_size = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("heap_caps_get_largest_free_block =  %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    // Initialize  ********************************************************
    i2s_std_init(&rx_handle);  
    init_sd();

    // Create model **********************************************************
    ESP_LOGI(TAG, "Creating model");
    Model *model = new Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION);
    ESP_LOGI(TAG, "Model created");

    // Capture mic audio with i2s ********************************************
    printf("heap_caps_get_free_size = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("heap_caps_get_largest_free_block =  %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    int waveformLength = 16000;
    printf("waveform size required: %d\n", waveformLength * sizeof(float));
    float *waveform = (float *)heap_caps_malloc(waveformLength * sizeof(float), MALLOC_CAP_8BIT);
    if (!waveform) {
        ESP_LOGE(TAG, "Failed to allocate memory for waveform");
        return;
    }
    printf("heap_caps_get_free_size = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("heap_caps_get_largest_free_block =  %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    // Record audio to both wav file on sd and waveform buffer
    record_wav_buffer(MOUNT_POINT"/model.wav", rx_handle, 16000, 1000000, waveform, waveformLength);
    ESP_LOGI(TAG, "1");

    // Generate spectrogram **************************************************
    int n_fft = 512;
    int win_length = n_fft;
    int hop_length = win_length / 2;

    int normalized = 0;
    int center = 0;
    int onesided = 1;

    float *spectrogram_output;
    int n_freq, n_time;

    spectrogram(waveform, waveformLength, n_fft, 
            hop_length, win_length, normalized, center, onesided, 
            &spectrogram_output, &n_freq, &n_time);
    heap_caps_free(waveform);

    // Transpose spectrogram **************************************************
    float *modelInput = (float *)heap_caps_malloc(n_freq * n_time * sizeof(float), MALLOC_CAP_8BIT);
    transpose(spectrogram_output, modelInput, n_freq, n_time);
    heap_caps_free(spectrogram_output);
    // Write spectrogram to file **********************************************
    printf("Before normalize: %f, %f, %f, %f\n", modelInput[0], modelInput[1], modelInput[2], modelInput[3]);
    printf("Before normalize: ...... %f, %f, %f, %f\n", modelInput[15612], modelInput[15613], modelInput[15614], modelInput[15615]);
    writeSpectrogram(modelInput, n_time, n_freq);
    // Normalize spectrogram *************************************************
    normalize_audio(modelInput, n_freq * n_time);
    printf("After normalize: %f, %f, %f, %f\n", modelInput[0], modelInput[1], modelInput[2], modelInput[3]);
    printf("After normalize: ...... %f, %f, %f, %f\n", modelInput[15612], modelInput[15613], modelInput[15614], modelInput[15615]);
    ESP_LOGI(TAG, "2");
    ESP_LOGI(TAG, "3");
    
    ESP_LOGI(TAG, "a");

    // Run model inference **************************************************
    // Create a shape vector
    std::vector<int> shape = {1, 256, 61};

    // Create the TensorBase object
    TensorBase* test_tensor = new TensorBase(shape, modelInput, 0, dl::DATA_TYPE_FLOAT, false, MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "b");

    // Create the map and insert the TensorBase object
    std::map<std::string, TensorBase*> test_inputs;
    ESP_LOGI(TAG, "c");
    test_inputs.emplace("input.1", test_tensor);
    ESP_LOGI(TAG, "d");
    model->run(test_inputs);
    ESP_LOGI(TAG, "e");
    std::map<std::string, TensorBase*> infer_outputs = model->get_outputs();
    log_model_outputs(infer_outputs);

    // Free memory **********************************************************
    heap_caps_free(waveform);
    delete test_tensor;

    delete model;
    ESP_LOGI(TAG, "exit app_main");
}
