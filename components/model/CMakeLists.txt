idf_component_register(SRCS "model.cpp"
                    REQUIRES "esp_driver_i2s" "spectrogram" "sd_card" "esp-dl" 
                    INCLUDE_DIRS "include"
                    )
