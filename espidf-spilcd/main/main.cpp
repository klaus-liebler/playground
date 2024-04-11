#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <math.h>
#include "sdkconfig.h"
#include <array>
#include <esp_log.h>
#include <spilcd16.hh>
#include <textrenderer.hh>
#define TAG "MAIN"



using namespace SPILCD16;
extern "C" void app_main(void)
{
    //M(spi_host_device_t spiHost, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl)
    char* text;
    asprintf(&text, "Simon %d", 42);
    SPILCD16::TextRenderer* tr = new TextRenderer(&lv_font_montserrat_16, Point2D(52, 100), RGB565::BLACK, RGB565::YELLOW, text);

    SPILCD16::M<SPI2_HOST, GPIO_NUM_7, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_NC, GPIO_NUM_18, LCD135x240> lcd;
    lcd.InitSpiAndGpio();
    lcd.Init_ST7789(RGB565::YELLOW);
    lcd.DrawAsyncAsSync(tr);
    while(true){
        lcd.Loop();
        //SPILCD16::FilledRectRenderer rr(Point2D(52,40), Point2D(72, 60), RGB565::GREEN);
        //lcd.DrawAsyncAsSync(&rr);
        //vTaskDelay(pdMS_TO_TICKS(500));
        //SPILCD16::FilledRectRenderer bb(Point2D(52,40), Point2D(72, 60), RGB565::BLUE);
        //lcd.DrawAsyncAsSync(&bb);
        vTaskDelay(pdMS_TO_TICKS(500));
    }  
}
