#include <driver/i2c_master.h>
#define TAG "MAIN"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = 0,
        .sda_io_num = 4,
        .scl_io_num = 5,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 0,
        .intr_priority = 3,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        }};
    i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    i2c_master_bus_reset(bus_handle);
    int i=0x1A;
    if (i2c_master_probe(bus_handle, i, 100) != ESP_ERR_NOT_FOUND)
    {
        ESP_LOGI(TAG, "Found I2C-Device @ 0x%02X", i);
    }
    return;
    
    for (uint8_t i = 1; i < 128; i++)
    {
        if (i2c_master_probe(bus_handle, i, 100) != ESP_ERR_NOT_FOUND)
        {
            ESP_LOGI(TAG, "Found I2C-Device @ 0x%02X", i);
        }
    }
}
