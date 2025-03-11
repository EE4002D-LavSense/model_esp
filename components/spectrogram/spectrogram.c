#include "spectrogram.h"



/**
 * Function to compute magnitude spectrogram using STFT
 */
__attribute__((aligned(16)))
float window[512];
__attribute__((aligned(16)))
float frame[512 * 2];

void spectrogram(float *waveform, int waveform_length, int n_fft, 
        int hop_length, int win_length, int normalized, int center, int onesided, 
        float **spectrogram_output, int *n_freq, int *n_time) {
    // Init
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    if (ret  != ESP_OK) {
        ESP_LOGE(tag, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }

    // Generate Hann window (reduces spectral leakage)
    //float *window = (float *)malloc(win_length * sizeof(float));
    dsps_wind_hann_f32(window, win_length);    
    ESP_LOGW(tag, "a");
    ESP_LOGW(tag, "b");

    // Determine the number of frames (time frames)
    int n_frames = center ?
            1 + waveform_length / hop_length : 
            1 + (waveform_length - n_fft) / hop_length;
    *n_time = n_frames;
    printf("Number of frames: %d\n", n_frames);

    // Determine the number of frequency bins
    int n_freqBins = onesided ? (n_fft / 2) : n_fft;
    *n_freq = n_freqBins;
    printf("Number of frequency bins: %d\n", n_freqBins);

    // Allocate memory for the spectrogram output
    printf("heap_caps_get_free_size = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("heap_caps_get_largest_free_block =  %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    printf("Spectrogram required size = %d\n", n_freqBins * n_frames * sizeof(float));
    *spectrogram_output = (float *)heap_caps_malloc(n_freqBins * n_frames * sizeof(float), MALLOC_CAP_8BIT);
    printf("Available space after allocating memory = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    ESP_LOGW(tag, "c");

    //float *frame = (float *)malloc(win_length * 2 * sizeof(float));
    for (int i = 0; i < n_frames; i++) {
        //ESP_LOGW(tag, "c1");
        for (int j = 0; j < win_length; j++) {
            frame[j * 2 + 0] = waveform[i * hop_length + j] * window[j];
            frame[j * 2 + 1] = 0.0;
        }

        // Compute the FFT
        dsps_fft2r_fc32(frame, n_fft);
        //ESP_LOGW(tag, "c2");
        // Processing FFT output
        dsps_bit_rev_fc32(frame, n_fft);
        dsps_cplx2reC_fc32(frame, n_fft);
        //ESP_LOGW(tag, "c3");

        // Compute the magnitude of the FFT
        for (int j = 0; j < n_freqBins; j++) {
            float real = frame[2 * j];
            float imag = frame[2 * j + 1];
            (*spectrogram_output)[i * n_freqBins + j] = 10 * log10f((real * real + imag * imag) / n_fft) + 20;
        }
        //ESP_LOGW(tag, "c4");

        /*
        // Visualising fft of each frame
        for (int j = 0; j < n_freqBins; j++) {
            frame[j] = 10 * log10f((frame[j*2+0] * frame[j*2+0] + frame[j*2+1] * frame[j*2+1]) / n_fft);
        }
        dsps_view(frame, n_freqBins, 64, 10, -60, 40, '|');
        */
    }
    ESP_LOGW(tag, "d");
    //free(frame);
    //free(window);
    ESP_LOGW(tag, "d1");

    // Normalize the spectrogram if required
    if (normalized) {
        float window_sum = 0.0f;
        for (int i = 0; i < win_length; i++) {
            window_sum += window[i] * window[i];
        }
        float norm_factor = sqrtf(window_sum);
        for (int i = 0; i < (*n_freq) * n_frames; i++) {
            (*spectrogram_output)[i] /= norm_factor;
        }
    }
    ESP_LOGW(tag, "e");
}

void transpose(float *arr, float *transposed, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transposed[j * rows + i] = arr[i * cols + j];  // Swap indices
        }
    }
}

