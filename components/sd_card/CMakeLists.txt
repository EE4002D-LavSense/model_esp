idf_component_register(SRCS "sd_card.c" "WAVFileWriter.cpp"
                    REQUIRES "fatfs" "esp_timer" "esp_driver_i2s"
                    INCLUDE_DIRS "include")
