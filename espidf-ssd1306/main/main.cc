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
#include "ssd1306_menu.hh"
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "ssd1306_fonts.hh"

using namespace ssd1306;
using namespace ssd1306::menu;

constexpr gpio_num_t SCL{GPIO_NUM_6};
constexpr gpio_num_t SDA{GPIO_NUM_7};
i2c_master_bus_handle_t bus_handle{nullptr};
i2c_master_dev_handle_t dev_handle{nullptr};

class :public MenuItemChanged<int>{
    public:
    void ValueChanged(const MenuItem* item, int newValue) override{
        ESP_LOGI(TAG, "Value of '%s' changed to %d", item->GetName(), newValue);
    }
} int_cb;

class :public MenuItemChanged<bool>{
    public:
    void ValueChanged(const MenuItem* item, bool newValue) override{
        ESP_LOGI(TAG, "Value of '%s' changed to %d", item->GetName(), newValue);
    }
} bool_cb;

class :public MenuItemChanged<float>{
    public:
    void ValueChanged(const MenuItem* item, float newValue) override{
        ESP_LOGI(TAG, "Value of '%s' changed to %f", item->GetName(), newValue);
    }
} float_cb;

ssd1306::M<128, 32> *lcd{nullptr};


int Integer2_1_0{1};
int Integer2_0{2};
int Integer2_2{3};
int Integer2_3{4};
int Integer0{0};
int Integer5{5};
bool Bool3{false};
float Fixed1{0.25};

std::vector<MenuItem *> folder2_1_items = {
    new IntegerItem("Integer2_1_0", &Integer2_1_0, 0, 10, &int_cb),
    new ReturnItem(),
};

std::vector<MenuItem *> folder2_items = {
    new IntegerItem("Integer2_0", &Integer2_0, 0, 10, &int_cb),
    new FolderItem("Folder2_1", &folder2_1_items),
    new IntegerItem("Integer2_2", &Integer2_2, 0, 10, &int_cb),
    new IntegerItem("Integer2_3", &Integer2_3, 0, 10, &int_cb),
    new ReturnItem(),
};

std::vector<const char *> option_items = {
    "Opt1",
    "Opt2",
    "Opt3",
    "Opt4",
    "Opt5",
};

std::vector<MenuItem *> root_items = {
    new IntegerItem("Integer0", &Integer0, 0, 10, &int_cb),
    new FixedPointItem<4>("Fixed1", &Fixed1, 0, 1, &float_cb),
    new FolderItem("Folder2", &folder2_items),
    new BoolItem("Bool3", &Bool3, &bool_cb),
    new OptionItem("Option", &option_items, &int_cb),
    new IntegerItem("Integer5", &Integer5, 0, 10, &int_cb),
    new PlaceholderItem("----------"),
};

enum class ButtonPressResult{
    NO_CHANGE,
    PRESSED,
    RELEASED_SHORT,
    PRESSED_LONG,
    RELEASED_LONG,
};

time_t pressed_time{INT64_MAX};
bool pressed_state{true};
bool pressed_long_already_sent{false};

static ButtonPressResult Button(){
    bool newstate = gpio_get_level(GPIO_NUM_0);
    time_t now =esp_timer_get_time();
    if(pressed_state && !newstate){//pressed down
        pressed_state=newstate;
        pressed_time=esp_timer_get_time();
        pressed_long_already_sent=false;
        ESP_LOGD(TAG, "Pressed!");
        return ButtonPressResult::PRESSED;
    }else if(!pressed_state && !newstate && !pressed_long_already_sent && now-pressed_time>400*1000){
        pressed_state=newstate;
        pressed_long_already_sent=true;
        ESP_LOGD(TAG, "Pressed long!");
        return ButtonPressResult::PRESSED_LONG;
    }
    else if(!pressed_state && newstate){
        pressed_state=newstate;
        pressed_time=INT64_MAX;
        ESP_LOGD(TAG, "Released!");
        return pressed_long_already_sent?ButtonPressResult::RELEASED_LONG:ButtonPressResult::RELEASED_SHORT;
    }
    return ButtonPressResult::NO_CHANGE;
}


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

    lcd = new ssd1306::M<128,32>(&font::Font16x8);
    lcd->Init(bus_handle);
    auto root_folder = new FolderItem("root", &root_items);
    auto m=new MenuManagement(root_folder, lcd);
    m->Init();
    while (true)
    {
        ButtonPressResult r = Button();
        switch (r)
        {
        case ButtonPressResult::RELEASED_SHORT:
            m->Down();
            break;
        case ButtonPressResult::PRESSED_LONG:
            m->Select();
            break;
        default:
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}