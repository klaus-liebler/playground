#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <math.h>
#include "sdkconfig.h"
#include <array>
#include <esp_log.h>
#include "spilcd16.hh"
#include "FullTextLineRenderer.hh"
#include "breakout_renderer.hh"
#include "lcd_font.hh"
#include "fonts/sans12pt1bpp.hh"
#include "C:/repos/labathome/labathome_firmware_stm32arduino/src/stm32_esp32_communication.hh"

#include <RGB565.hh>
#define TAG "MAIN"

/*
class : public MenuItemChanged<int>
{
public:
    void ValueChanged(const MenuItem *item, int newValue) override
    {
        ESP_LOGI(TAG, "Value of '%s' changed to %d", item->GetName(), newValue);
    }
} int_cb;

class : public MenuItemChanged<bool>
{
public:
    void ValueChanged(const MenuItem *item, bool newValue) override
    {
        ESP_LOGI(TAG, "Value of '%s' changed to %d", item->GetName(), newValue);
    }
} bool_cb;

class : public MenuItemChanged<float>
{
public:
    void ValueChanged(const MenuItem *item, float newValue) override
    {
        ESP_LOGI(TAG, "Value of '%s' changed to %f", item->GetName(), newValue);
    }
} float_cb;

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

std::vector<const char *> option4_items = {
    "Opt1",
    "Opt2",
    "Opt3",
    "Opt4",
    "Opt5",
};

std::vector<const char *> option6_items = {
    G_CAMERA "Cam",
    G_BOOK "Book",
    G_FILM "Film",
    G_FIRE "Fire",
    G_BICYCLE "Bike",
};

std::vector<MenuItem *> root_items = {
    new IntegerItem("Integer0", &Integer0, 0, 10, &int_cb),
    new FixedPointItem<4>("Fixed1", &Fixed1, 0, 1, &float_cb),
    new FolderItem("Folder2", &folder2_items),
    new BoolItem("Bool3", &Bool3, &bool_cb),
    new OptionItem("Option4", &option4_items, &int_cb),
    new IntegerItem("Integer5", &Integer5, 0, 10, &int_cb),
    new OptionItem("Option6", &option6_items, &int_cb),
    new PlaceholderItem("Ph7\t***\tPh7"),
};
*/
constexpr i2c_port_t I2C_PORT{I2C_NUM_0};
constexpr gpio_num_t PIN_BTN_GREEN = (gpio_num_t)0;
constexpr gpio_num_t PIN_LCD_DC = (gpio_num_t)8;
constexpr gpio_num_t PIN_LCD_CLK = (gpio_num_t)17;
constexpr gpio_num_t PIN_LCD_DAT = (gpio_num_t)18;
constexpr gpio_num_t PIN_LCD_BL = (gpio_num_t)38;
constexpr gpio_num_t PIN_EXT_IO1 = (gpio_num_t)11;

constexpr gpio_num_t PIN_I2C_SDA = (gpio_num_t)4;
constexpr gpio_num_t PIN_I2C_SCL = (gpio_num_t)5;

spilcd16::M<SPI2_HOST, PIN_LCD_DAT, PIN_LCD_CLK, GPIO_NUM_NC, PIN_LCD_DC, PIN_EXT_IO1, GPIO_NUM_NC, LCD240x240_0, (size_t)8 * 240, 4096, 0> lcd;
spilcd16::FullTextlineRenderer<32, 240, 5, 5, 24> *lineRenderer{nullptr};
BREAKOUT::Renderer<240, 240> *breakout{nullptr};

i2c_master_bus_handle_t i2c_master_handle{nullptr};
i2c_master_dev_handle_t stm32_handle{nullptr};
uint8_t stm2esp_buf[S2E::SIZE] = {0};
uint8_t esp2stm_buf[E2S::SIZE] = {0};

extern "C" void app_main(void)
{

    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        }};

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_master_handle));
    ESP_ERROR_CHECK(i2c_master_probe(i2c_master_handle, I2C_SETUP::STM32_I2C_ADDRESS, 1000));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_SETUP::STM32_I2C_ADDRESS,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0,
        },
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_master_handle, &dev_cfg, &stm32_handle));
    lcd.InitSpiAndGpio();
    lcd.Init_ST7789(Color::BLACK);
    breakout = new BREAKOUT::Renderer<240, 240>(&sans12pt1bpp::font);
    breakout->GameInit(&lcd);
    TickType_t pxPreviousWakeTime{0};
    uint16_t enc_old{0};
    while (true)
    {
        i2c_master_receive(stm32_handle, stm2esp_buf, S2E::SIZE, 1000);
        uint16_t enc=ParseU16(stm2esp_buf, S2E::ROTENC_POS);
        int diff = (int)enc-(int)enc_old;
        breakout->GameLoop(diff*2, gpio_get_level(PIN_BTN_GREEN)==0, &lcd);
        lcd.Draw(breakout);
        enc_old=enc;
        xTaskDelayUntil(&pxPreviousWakeTime, 5);
    }
}
