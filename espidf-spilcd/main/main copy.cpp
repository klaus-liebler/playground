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
#define TAG "MAIN"


#define LCD_HOST       SPI2_HOST
#define PARALLEL_LINES 16
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  0
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_MOSI          33
#define EXAMPLE_PIN_NUM_SCLK           32
#define EXAMPLE_PIN_NUM_CS             22
#define EXAMPLE_PIN_NUM_DC             15
#define EXAMPLE_PIN_NUM_RST            21
#define EXAMPLE_PIN_NUM_BK_LIGHT       ((gpio_num_t)5)
#define EXAMPLE_LCD_H_RES              135
#define EXAMPLE_LCD_V_RES              240
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */

esp_lcd_panel_handle_t panel_handle = NULL;
size_t pixelsCnt=EXAMPLE_LCD_H_RES * PARALLEL_LINES;
int row=-1;
uint16_t *colorData;
constexpr uint16_t Swap2Bytes(uint16_t val){return ((((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00));}
constexpr uint16_t RGBto565(uint8_t r, uint8_t g, uint8_t b){return ((((r)&0xF8) << 8) | (((g)&0xFC) << 3) | ((b) >> 3));}

uint16_t palette[]={TFT_SILVER, TFT_RED, TFT_GREEN, TFT_BLUE};
extern "C" bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx){
    if(row==-1) return false;
    if(row==15) return false;
    //ESP_LOGI(TAG, "on_color_trans_done");
    row++;
    for(size_t i=0;i<pixelsCnt;i++){
        colorData[i]=Swap2Bytes(palette[row%4]);
    }
    assert(panel_io!=NULL);
    esp_lcd_panel_io_tx_color(panel_io, LCD_CMD_RAMWR, colorData, pixelsCnt*2);
    return false;
}

extern "C" void app_main(void)
{
    gpio_config_t bk_gpio_config = {};
    bk_gpio_config.mode = GPIO_MODE_OUTPUT;
    bk_gpio_config.pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT;
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num = EXAMPLE_PIN_NUM_SCLK;
    buscfg.mosi_io_num = EXAMPLE_PIN_NUM_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = PARALLEL_LINES * EXAMPLE_LCD_H_RES * 2 + 8;
    buscfg.intr_flags=0;
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.dc_gpio_num = EXAMPLE_PIN_NUM_DC;
    io_config.cs_gpio_num = EXAMPLE_PIN_NUM_CS;
    io_config.pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ;
    io_config.lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS;
    io_config.lcd_param_bits = EXAMPLE_LCD_PARAM_BITS;
    io_config.spi_mode = 3;
    io_config.on_color_trans_done=on_color_trans_done;
    io_config.trans_queue_depth = 10;

    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num = EXAMPLE_PIN_NUM_RST;
    panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
    panel_config.bits_per_pixel = 16;
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL));
    // Allocate memory for the pixel buffers

    colorData = (uint16_t*)heap_caps_malloc(pixelsCnt * sizeof(uint16_t), MALLOC_CAP_DMA);
    uint16_t BUFFER[256];
    for(size_t x=0;x<256;x++){
        BUFFER[x]=Swap2Bytes(TFT_WHITE);
    }
    
    for(int row=0;row<320;row++){
        esp_lcd_panel_draw_bitmap(panel_handle, 0, row, 240, row+1, BUFFER);
    }
    //vTaskDelay(pdMS_TO_TICKS(1000));
    row=0;
    for(size_t i=0;i<pixelsCnt;i++){
        colorData[i]=Swap2Bytes(palette[row%4]);
    }
    int x_start=52;
    int x_end= 52+135;
    int y_start=40;
    int y_end=280;
        // define an area of frame memory where MCU can access
    uint8_t paramsCASET[4]= {
        (uint8_t)((x_start >> 8) & 0xFF),
        (uint8_t)(x_start & 0xFF),
        (uint8_t)(((x_end - 1) >> 8) & 0xFF),
        (uint8_t)((x_end - 1) & 0xFF),
    };
    esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_CASET, paramsCASET, 4);
    uint8_t paramsRASET[4]={
        (uint8_t)((y_start >> 8) & 0xFF),
        (uint8_t)(y_start & 0xFF),
        (uint8_t)(((y_end - 1) >> 8) & 0xFF),
        (uint8_t)((y_end - 1) & 0xFF),
    };
    esp_lcd_panel_io_tx_param(io_handle, LCD_CMD_RASET, paramsRASET, 4);
    // transfer frame buffer
    esp_lcd_panel_io_tx_color(io_handle, LCD_CMD_RAMWR, colorData, pixelsCnt*2);

    while(true){
        vTaskDelay(10);
    }  
}
