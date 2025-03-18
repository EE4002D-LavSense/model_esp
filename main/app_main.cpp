#include "dl_model_base.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "fbs_loader.hpp"
#include <type_traits>
#include "esp_heap_caps.h"
#include "driver/i2s_std.h"

#include "mic.h"
#include "sd_card.h"
#include "model.h"

static const char *TAG = "APP MAIN";

// using namespace fbs;
using namespace dl;

// *****************************
// I2S 
// *****************************
i2s_chan_handle_t rx_handle;


// *****************************
// Main
// *****************************

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "get into app_main");
    printf("heap_caps_get_free_size = %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("heap_caps_get_largest_free_block =  %d\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    // Initialize  ********************************************************
    i2s_std_init(&rx_handle);  
    if (init_sd() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD card");
        return;
    }

    // Create model **********************************************************
    ESP_LOGI(TAG, "Creating model");
    Model *model = new Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION);
    ESP_LOGI(TAG, "Model created");

    // Test variables ******************************************************
    bool doorClose = false;
    bool waterFlow = false;

    // Run inference ********************************************************
    ESP_LOGI(TAG, "Running inference");
    modelInference(model, rx_handle, &doorClose, &waterFlow);
    ESP_LOGI(TAG, "Inference complete. Door close: %d, Water flow: %d", doorClose, waterFlow);

    delete model;
    ESP_LOGI(TAG, "exit app_main");
}
