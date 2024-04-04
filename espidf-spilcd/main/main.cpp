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
#define TAG "MAIN"


#define LCD_HOST       HSPI_HOST
#define PARALLEL_LINES 16
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  0
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_MOSI          7
#define EXAMPLE_PIN_NUM_SCLK           15
#define EXAMPLE_PIN_NUM_CS             -1
#define EXAMPLE_PIN_NUM_DC             16
#define EXAMPLE_PIN_NUM_RST            6
#define EXAMPLE_PIN_NUM_BK_LIGHT       ((gpio_num_t)5)
#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              240
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8



using namespace SPILCD16;
extern "C" void app_main(void)
{
    //M(spi_host_device_t spiHost, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl)
    SPILCD16::M<SPI2_HOST, GPIO_NUM_7, GPIO_NUM_15, GPIO_NUM_NC, GPIO_NUM_16, GPIO_NUM_6, GPIO_NUM_5, 240, 240, 0, 0> manager;
    manager.Init_ST7789(RGB565::BLACK);
    while(true){
        manager.Loop();
        SPILCD16::FilledRectRenderer rr(Point2D(52,40), Point2D(72, 60), RGB565::RED);
        manager.DrawAsyncAsSync(&rr);
        vTaskDelay(pdMS_TO_TICKS(250));
        SPILCD16::FilledRectRenderer bb(Point2D(52,40), Point2D(72, 60), RGB565::BLUE);
        manager.DrawAsyncAsSync(&bb);
        vTaskDelay(pdMS_TO_TICKS(250));
    }  
}
