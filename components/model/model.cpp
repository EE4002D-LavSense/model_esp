#include "model.h"

extern "C" void modelInference(Model *model, i2s_chan_handle_t rx_handle) {
    // Capture mic audio with i2s ********************************************
    int waveformLength = 16000;
    float *waveform = (float *)heap_caps_malloc(waveformLength * sizeof(float), MALLOC_CAP_8BIT);

    // Record audio to both wav file on sd and waveform buffer
    record_wav_buffer(MOUNT_POINT"/model.wav", rx_handle, 16000, 1000000, waveform, waveformLength);

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

    // Run model inference **************************************************
    // Create a shape vector
    std::vector<int> shape = {1, 256, 61};

    // Create the TensorBase object
    TensorBase* test_tensor = new TensorBase(shape, modelInput, 0, dl::DATA_TYPE_FLOAT, false, MALLOC_CAP_SPIRAM);
    ESP_LOGI(MODEL_TAG, "b");

    // Create the map and insert the TensorBase object
    std::map<std::string, TensorBase*> test_inputs;
    ESP_LOGI(MODEL_TAG, "c");
    test_inputs.emplace("input.1", test_tensor);
    ESP_LOGI(MODEL_TAG, "d");
    model->run(test_inputs);
    ESP_LOGI(MODEL_TAG, "e");
    std::map<std::string, TensorBase*> infer_outputs = model->get_outputs();
    log_model_outputs(infer_outputs);

    // Free memory **********************************************************
    heap_caps_free(waveform);
    delete test_tensor;
}

// Function to print the contents of a TensorBase using ESP_LOGI
extern "C" void log_tensor_data(const std::string& tensor_name, dl::TensorBase* tensor) {
    if (!tensor) {
        ESP_LOGE(MODEL_TAG, "Tensor %s is null and cannot be printed.", tensor_name.c_str());
        return;
    }

    std::vector<int> shape = tensor->get_shape();
    std::string shape_str;
    for (int dim : shape) {
        shape_str += std::to_string(dim) + " ";
    }
    //ESP_LOGI(MODEL_TAG, "Tensor name: %s, shape: [%s]", tensor_name.c_str(), shape_str.c_str());

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
        ESP_LOGI(MODEL_TAG, "Data (int8): %s", data_str.c_str());
    } else if (tensor->dtype == dl::DATA_TYPE_INT16) {
        const int16_t* data = static_cast<const int16_t*>(tensor->data);
        std::string data_str;
        for (size_t i = 0; i < num_elements; ++i) {
            data_str += std::to_string(data[i]) + " ";
        }
        ESP_LOGI(MODEL_TAG, "Data (int16): %s", data_str.c_str());
    } else if (tensor->dtype == dl::DATA_TYPE_FLOAT) {
        const float* data = static_cast<const float*>(tensor->data);
        std::string data_str;
        for (size_t i = 0; i < num_elements; ++i) {
            data_str += std::to_string(data[i]) + " ";
        }
        ESP_LOGI(MODEL_TAG, "Data (float): %s", data_str.c_str());

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
        ESP_LOGI(MODEL_TAG, "Class: %d, Max value: %f", max_index, max_value);
    } else {
        ESP_LOGE(MODEL_TAG, "Unsupported data type for tensor %s.", tensor_name.c_str());
    }
}

// Function to iterate over model outputs and log them
extern "C" void log_model_outputs(std::map<std::string, TensorBase*> model_outputs) {
    for (const auto& output_pair : model_outputs) {
        const std::string& output_name = output_pair.first;
        TensorBase* output_tensor = output_pair.second;
        log_tensor_data(output_name, output_tensor);
    }
}
