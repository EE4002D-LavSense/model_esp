#include "esp_vfs_fat.h"
#include "WAVFileWriter.hpp"
#include "esp_timer.h"
#include "driver/i2s_std.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/sdcard"
#define SD_TAG "SD_CARD"

#define PIN_NUM_MISO  GPIO_NUM_4
#define PIN_NUM_MOSI  GPIO_NUM_5
#define PIN_NUM_CLK   GPIO_NUM_6
#define PIN_NUM_CS    GPIO_NUM_7

#define I2SBUFFERSIZE 1024


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SD card
 */
void init_sd();


/**
 * @brief Record audio data from the I2S microphone for a specified length and save it to a WAV file
 * 
 * @param usLength Length of the recording in microseconds (1hr - 3600000000, 30 mins - 1800000000,30s - 30000000, 5min - 300000000, 5s - 5000000)
 */
void record_wav(const char *fname, i2s_chan_handle_t rx_handle, int sampleRate, int usLength);


/**
 * @brief Record audio data from the I2S microphone for a specified length,
 * saving it to a wav file and in a float buffer for on board processing
 * 
 * @param usLength Length of the recording in microseconds (1hr - 3600000000, 30 mins - 1800000000,30s - 30000000, 5min - 300000000, 5s - 5000000)
 */
void record_wav_buffer(const char *fname, i2s_chan_handle_t rx_handle, int sampleRate, int usLength, 
        float *waveform, int waveform_length);


#ifdef __cplusplus
}
#endif
