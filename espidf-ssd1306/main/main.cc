#include <cstdio>
#include <cstdint>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include "ssd1306.hh"
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "klfont.hh"
#include "symbols.hh"

using namespace ssd1306;

constexpr gpio_num_t SCL{GPIO_NUM_6};
constexpr gpio_num_t SDA{GPIO_NUM_7};
i2c_master_bus_handle_t bus_handle{nullptr};
i2c_master_dev_handle_t dev_handle{nullptr};



ssd1306::M<128, 64> *lcd{nullptr};


extern "C" void app_main(void)
{
   
   
    i2c_master_bus_config_t i2c_mst_config = {};

    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.i2c_port = -1;
    i2c_mst_config.scl_io_num = SCL;
    i2c_mst_config.sda_io_num = SDA;
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
    // ESP_ERROR_CHECK(i2c_master_probe(bus_handle, SSD1306_I2C_ADDRESS, 100));
    // ESP_LOGI(TAG, "Found SSD1306");

    // i2c_device_config_t dev_cfg = {};
    // dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    // dev_cfg.device_address = SSD1306_I2C_ADDRESS;
    // dev_cfg.scl_speed_hz = 100000;
    // ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    lcd = new ssd1306::M<128, 64>(&arial_and_symbols_16px1bpp::font);
    lcd->Init(bus_handle);
    vTaskDelay(pdMS_TO_TICKS(100));
    lcd->printfl(0, false, "Hallo " G_VOLUME_2);
}