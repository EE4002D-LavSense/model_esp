#include "spectrogram.h"

/**
 * Function to compute magnitude spectrogram using STFT
 * 
 * @return spectrogram_output: pointer to the spectrogram output with shape (n_time, n_freq)
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
        ESP_LOGE(SPEC_TAG, "Not possible to initialize FFT. Error = %i", ret);
        return;
    }

    // Generate Hann window (reduces spectral leakage)
    //float *window = (float *)malloc(win_length * sizeof(float));
    dsps_wind_hann_f32(window, win_length);    
    ESP_LOGW(SPEC_TAG, "a");
    ESP_LOGW(SPEC_TAG, "b");

    // Determine the number of frames (time frames)
    int n_frames = center ?
            1 + waveform_length / hop_length : 
            1 + (waveform_length - n_fft) / hop_length;
    *n_time = n_frames;
    ESP_LOGI(SPEC_TAG,"Number of frames (n_time): %d\n", n_frames);

    // Determine the number of frequency bins
    int n_freqBins = onesided ? (n_fft / 2) : n_fft;
    *n_freq = n_freqBins;
    ESP_LOGI(SPEC_TAG, "Number of frequency bins (n_freq): %d\n", n_freqBins);

    // Allocate memory for the spectrogram output
    *spectrogram_output = (float *)heap_caps_malloc(n_freqBins * n_frames * sizeof(float), MALLOC_CAP_8BIT);

    //float *frame = (float *)malloc(win_length * 2 * sizeof(float));
    for (int i = 0; i < n_frames; i++) {
        //ESP_LOGW(SPEC_TAG, "c1");
        for (int j = 0; j < win_length; j++) {
            frame[j * 2 + 0] = waveform[i * hop_length + j] * window[j];
            frame[j * 2 + 1] = 0.0;
        }

        // Compute the FFT
        dsps_fft2r_fc32(frame, n_fft);
        //ESP_LOGW(SPEC_TAG, "c2");
        // Processing FFT output
        dsps_bit_rev_fc32(frame, n_fft);
        dsps_cplx2reC_fc32(frame, n_fft);
        //ESP_LOGW(SPEC_TAG, "c3");

        // Compute the magnitude of the FFT
        for (int j = 0; j < n_freqBins; j++) {
            float real = frame[2 * j];
            float imag = frame[2 * j + 1];
            (*spectrogram_output)[i * n_freqBins + j] = 10 * log10f((real * real + imag * imag) / n_fft) + 20;
        }
        //ESP_LOGW(SPEC_TAG, "c4");

        /*
        // Visualising fft of each frame
        for (int j = 0; j < n_freqBins; j++) {
            frame[j] = 10 * log10f((frame[j*2+0] * frame[j*2+0] + frame[j*2+1] * frame[j*2+1]) / n_fft);
        }
        dsps_view(frame, n_freqBins, 64, 10, -60, 40, '|');
        */
    }
    ESP_LOGW(SPEC_TAG, "d");
    //free(frame);
    //free(window);
    ESP_LOGW(SPEC_TAG, "d1");

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
    ESP_LOGW(SPEC_TAG, "e");
}

void transpose(float *arr, float *transposed, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            transposed[j * rows + i] = arr[i * cols + j];  // Swap indices
        }
    }
}

void normalize_audio(float *aud, int size) {
    double sum = 0.0;
    float mean, std_dev = 0.0;

    // Compute mean
    for (int i = 0; i < size; i++) {
        sum += aud[i];
    }
    mean = (float) (sum / size);

    // Compute standard deviation
    sum = 0;
    for (int i = 0; i < size; i++) {
        sum += (aud[i] - mean) * (aud[i] - mean);
    }
    std_dev = (float) sqrt(sum / size);

    // Normalize audio
    for (int i = 0; i < size; i++) {
        aud[i] = (aud[i] - mean) / std_dev;
    }
}

