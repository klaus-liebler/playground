#include <cstdio>
#include <cstdint>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/i2c_master.h>
#include <ssd1306.hh>
#include <simple_menu.hh>
#include <single_button.hh>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#define TAG "MAIN"
#include <esp_log.h>


using namespace lcd_i2c_1bpp;
using namespace menu;
using namespace display;

menu::MenuManagement* m{nullptr};

#include "handler_and_menu_definition.inc"

constexpr gpio_num_t SCL{GPIO_NUM_6};
constexpr gpio_num_t SDA{GPIO_NUM_7};
i2c_master_bus_handle_t bus_handle{nullptr};
i2c_master_dev_handle_t dev_handle{nullptr};


lcd_i2c_1bpp::M<128, 64> *lcd{nullptr};


extern "C" void app_main(void)
{
    lcd_i2c_1bpp::CreateI2CMasterBusHandle(SCL, SDA, &bus_handle);
    
    lcd = new lcd_i2c_1bpp::M<128, 64>(&arial_and_symbols_16px1bpp::font);
    lcd->Init(bus_handle);
    
    
    auto root_folder = new FolderItem("root", &root_items);
    m = new menu::MenuManagement(root_folder, lcd);
    m->Init();
    while (true)
    {
        button::ButtonLoop(m, GPIO_NUM_0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}