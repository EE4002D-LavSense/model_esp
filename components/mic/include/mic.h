#include "esp_timer.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

// *****************************
// Constants
// *****************************
#define I2SBCLK         GPIO_NUM_9
#define I2SWS           GPIO_NUM_10
#define I2SDIN          GPIO_NUM_11
#define I2SBUFFERSIZE   1024
#define SAMPLERATE      16000

#define MIC_TAG "MIC"


// *****************************
// Functions
// *****************************

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the I2S microphone
 */
void i2s_std_init(i2s_chan_handle_t *rx_handle);

/**
 * @brief Record a specified length of audio data from the I2S microphone to a float buffer
 * 
 * @param usLength Length of the recording in microseconds (1hr - 3600000000, 30 mins - 1800000000,30s - 30000000, 5min - 300000000, 5s - 5000000)
 * @note Untested
 */
void record_buffer(i2s_chan_handle_t rx_handle, int sampleRate, int usLength, float *waveform, int waveform_length);

#ifdef __cplusplus
}
#endif
