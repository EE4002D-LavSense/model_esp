#include "sd_card.h"

int min(int a, int b) {
    return a < b ? a : b;
}

esp_err_t init_sd() {
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(SD_TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.
    ESP_LOGI(SD_TAG, "Using SPI peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 20MHz for SDSPI)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(SD_TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(SD_TAG, "Failed to initialize bus.");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(SD_TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SD_TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(SD_TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return ret;
    }
    ESP_LOGI(SD_TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return ret;
}


/**
 * @brief Record audio data from the I2S microphone for a specified length and save it to a WAV file
 * 
 * @param usLength Length of the recording in microseconds (1hr - 3600000000, 30 mins - 1800000000,30s - 30000000, 5min - 300000000, 5s - 5000000)
 */
void record_wav(const char *fname, i2s_chan_handle_t rx_handle, int sampleRate, int usLength) {
    int32_t *r_buf = (int32_t *)calloc(1, I2SBUFFERSIZE);
    assert(r_buf);
    size_t bytes_read = 0;

    // open the file on the sdcard
    FILE *fp = fopen(fname, "wb");
    // create a new wave file writer
    WAVFileWriter *writer = WAVFileWriter_C(fp, sampleRate);

    // Timing
    int64_t timeStart = esp_timer_get_time();

    while (esp_timer_get_time() - timeStart < usLength) {   
        if (i2s_channel_read(rx_handle, r_buf, I2SBUFFERSIZE, &bytes_read, 1000) == ESP_OK) {
            write_C(writer, r_buf, bytes_read / sizeof(int32_t));
        } else {
            ESP_LOGI(SD_TAG, "I2S read failed");
        }
        //vTaskDelay(pdMS_TO_TICKS(200));
    }
    finish_C(writer);
    fclose(fp);
    destroy_C(writer);
    free(r_buf);
}

/**
 * @brief Record audio data from the I2S microphone for a specified length,
 * saving it to a wav file and in a float buffer for on board processing
 * 
 * @param usLength Length of the recording in microseconds (1hr - 3600000000, 30 mins - 1800000000,30s - 30000000, 5min - 300000000, 5s - 5000000)
 */
void record_wav_buffer(const char *fname, i2s_chan_handle_t rx_handle, int sampleRate, int usLength, 
        float *waveform, int waveform_length) {
    int32_t *r_buf = (int32_t *)calloc(1, I2SBUFFERSIZE);
    assert(r_buf);
    size_t bytes_read = 0;
    ESP_LOGI(SD_TAG, "a");

    // open the file on the sdcard
    FILE *fp = fopen(fname, "wb");
    ESP_LOGI(SD_TAG, "a1");
    // create a new wave file writer
    WAVFileWriter *writer = WAVFileWriter_C(fp, sampleRate);
    ESP_LOGI(SD_TAG, "b");

    // Timing
    int64_t timeStart = esp_timer_get_time();

    int idx = 0;
    int samplesRequired = usLength / 1000000 * sampleRate;
    while (samplesRequired > 0) {  
        if (i2s_channel_read(rx_handle, r_buf, I2SBUFFERSIZE, &bytes_read, 1000) == ESP_OK) {
            int samplesRead = bytes_read / sizeof(int32_t);
            write_C(writer, r_buf, min(samplesRead, samplesRequired));
            //ESP_LOGI(SD_TAG, "val %d\n", (int)r_buf[0]);

            for (int i = 0; i < samplesRead; i++) {
                if (idx >= waveform_length) {
                    break;
                }
                waveform[idx] = (float)r_buf[i] / (float)INT32_MAX;
                if (idx < 3 || idx > waveform_length - 4) {
                    //ESP_LOGI(SD_TAG, "idx %d: raw val %d, float val %f\n", idx, (int)r_buf[i], waveform[idx]);
                }
                idx += 1;
            }
            samplesRequired -= samplesRead;
        } else {
            ESP_LOGI(SD_TAG, "I2S read failed");
        }
    }
    ESP_LOGI(SD_TAG, "d");
    finish_C(writer);
    fclose(fp);
    destroy_C(writer);
    free(r_buf);
    ESP_LOGI(SD_TAG, "e");
}

void writeSpectrogram(float *spectrogram_output, int n_freq, int n_time) {
    FILE *f = fopen(MOUNT_POINT"/spec.txt", "w");
    if (f == NULL) {
        ESP_LOGE(SD_TAG, "Failed to open file for writing");
        return;
    }

    for (int i = 0; i < n_freq; i++) {
        fprintf(f, "[");
        for (int j = 0; j < n_time; j++) {
            fprintf(f, "%f, ", spectrogram_output[i * n_time + j]);
        }
        fprintf(f, "]\n");
    }

    fclose(f);
    ESP_LOGI(SD_TAG, "File written successfully");
}
