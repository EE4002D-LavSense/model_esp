/**
 * Helper functions to compute mel spectrogram of audio waveforms
 * 
 * Algorithm from torchaudio:
 * Spectrogram: https://github.com/pytorch/audio/blob/f084f34bbb743fada85f66b0ed8041387565e69c/src/torchaudio/functional/functional.py#L57
 */

#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "esp_dsp.h"
#include "esp_heap_caps.h"

static const char *tag = "spectrogram";

/**
 * Parameters for STFT
#define SAMPLE_RATE 16000      // 16kHz sampling rate
#define N_FFT  1024           // FFT window size (must be power of 2)
#define HOP_LENGTH 0           // Hop length (overlap = FFT_SIZE - HOP_SIZE)
#define NUM_FRAMES 10          // Number of STFT frames to compute
#define WIN_LENGTH 1024         // Window size
 */

/**
 * Functions
 */

#ifdef __cplusplus
extern "C" {
#endif

void spectrogram(float *waveform, int waveform_length, int n_fft, 
        int hop_length, int win_length, int normalized, int center, int onesided, 
        float **spectrogram_output, int *n_freq, int *n_time);


void transpose(float *arr, float *transposed, int rows, int cols);

void normalize_audio(float *aud, int size);

#ifdef __cplusplus
}
#endif