#include "mic.h"

/**
 * @brief Initialize the I2S microphone
 */
void i2s_std_init(i2s_chan_handle_t rx_handle)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle));

    i2s_std_config_t rx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLERATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2SBCLK,
            .ws = I2SWS,
            .dout = I2S_GPIO_UNUSED,
            .din = I2SDIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; 

    /* Set the mclk_multiple to a multiple of 3, so that it can calculate an integer division to a 24-bit data */
    //rx_std_cfg.clk_cfg.mclk_multiple = 384;

    /* Initialize the channel */
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle, &rx_std_cfg));

    /* Before reading data, start the RX channel first */
    i2s_channel_enable(rx_handle);
}


/**
 * @brief Record a specified length of audio data from the I2S microphone to a float buffer
 * 
 * @param usLength Length of the recording in microseconds (1hr - 3600000000, 30 mins - 1800000000,30s - 30000000, 5min - 300000000, 5s - 5000000)
 * @note Untested
 */
void record_buffer(i2s_chan_handle_t rx_handle, int sampleRate, int usLength, float *waveform, int waveform_length) {
    int32_t *r_buf = (int32_t *)calloc(1, I2SBUFFERSIZE);
    assert(r_buf);
    size_t bytes_read = 0;

    // Timing
    int64_t timeStart = esp_timer_get_time();

    int idx = 0;
    while (esp_timer_get_time() - timeStart < usLength) {  
        if (i2s_channel_read(rx_handle, r_buf, I2SBUFFERSIZE, &bytes_read, 1000) == ESP_OK) {
            //ESP_LOGI(TAG, "val %d\n", (int)r_buf[0]);

            int samples_read = bytes_read / sizeof(int32_t);
            for (int i = 0; i < samples_read; i++) {
                if (idx >= waveform_length) {
                    break;
                }
                waveform[idx] = (float)r_buf[i] / (float)INT32_MAX;
                /*
                if (idx < 3 || idx > waveform_length - 4) {
                    ESP_LOGI(TAG, "idx %d: raw val %d, float val %f\n", idx, (int)r_buf[i], waveform[idx]);
                }
                */
                idx += 1;
            }
        } else {
            ESP_LOGI(MIC_TAG, "I2S read failed");
        }
    }
    free(r_buf);
}