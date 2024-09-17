#include <driver/i2c_master.h>
#include <driver/gpio.h>
#define TAG "MAIN"
#include "esp_log.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#define SCL GPIO_NUM_5
#define SDA GPIO_NUM_4
//SCL=EXT_CLK=10
//SDA=EXT_MISO=9
#define SCL GPIO_NUM_10
//#define SDA GPIO_NUM_9

void app_main(void)
{
    while(false){
        ESP_LOGI(TAG, "I do nothing");
        vTaskDelay(100);
    }
    
    gpio_set_direction(SCL, GPIO_MODE_OUTPUT_OD);
    for(int p=0; p<20;p++){
        gpio_set_level(SCL, 0);
        esp_rom_delay_us(20);
        gpio_set_level(SCL, 1);
        esp_rom_delay_us(20); 
    }
    
    
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = 0,
        .sda_io_num = SDA,
        .scl_io_num = SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 0,
        .intr_priority = 3,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        }};
    i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    i2c_master_bus_reset(bus_handle);
    while(true){
        int i=0x1A;
        if (i2c_master_probe(bus_handle, i, 100) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found I2C-Device @ 0x%02X", i);
        }
        i=0x38;
        if (i2c_master_probe(bus_handle, i, 100) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found I2C-Device @ 0x%02X", i);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
   
}
