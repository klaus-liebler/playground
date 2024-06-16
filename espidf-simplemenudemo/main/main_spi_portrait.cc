#include <cstdio>
#include <ctime>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include <esp_timer.h>
#include <math.h>
#include "sdkconfig.h"
#include <array>
#include <esp_log.h>
#include <spilcd16.hh>
#include "FullTextLineRenderer.hh"
#include "simple_menu_without_hw_scrolling.hh"
#include "single_button.hh"
#include <lcd_font.hh>

#include <RGB565.hh>
#define TAG "MAIN"
using namespace spilcd16;
using namespace menu;
using namespace display;

constexpr gpio_num_t PIN_LCD_CS{GPIO_NUM_12};
constexpr gpio_num_t PIN_LCD_RS{GPIO_NUM_13};
constexpr gpio_num_t PIN_LCD_SCLK{GPIO_NUM_14};
constexpr gpio_num_t PIN_LCD_MOSI{GPIO_NUM_21};
constexpr gpio_num_t PIN_LCD_BACKLIGHT{GPIO_NUM_45};
constexpr gpio_num_t PIN_BUTTON{GPIO_NUM_0};

#include "handler_and_menu_definition.inc"

template <uint8_t LINE_HEIGHT_PIXELS, uint8_t LINE_WIDTH_PIXELS, uint8_t PADDING_LEFT, uint8_t PADDING_RIGHT>
class LineWriterToSpiLcdAdapter : public iFullLineWriterWithoutHardwareScrolling
{
private:
    IRendererHost *host;
    FilledRectRenderer* frr;
    FullTextlineRenderer<LINE_HEIGHT_PIXELS, LINE_WIDTH_PIXELS, PADDING_LEFT, PADDING_RIGHT>* ftlr;
public:
    LineWriterToSpiLcdAdapter(IRendererHost *host) : host(host) {
        ftlr= new FullTextlineRenderer<LINE_HEIGHT_PIXELS, LINE_WIDTH_PIXELS, PADDING_LEFT, PADDING_RIGHT>(&arial_and_symbols_24px1bpp::font);
    }
    
    size_t printfl(int line, bool invert, const char *format, ...) override
    {
        va_list args_list;
        va_start(args_list, format);
        line=(line+GetShownLines())%GetShownLines();
        size_t ret = ftlr->printfl(line, invert?Color::BLACK:Color::YELLOW, invert?Color::YELLOW:Color::BLACK, format, args_list);
        va_end(args_list);
        host->Draw(ftlr, true);
        return ret; 
    }

    void clearLines(int startLineIncl, int endLineExc) override{
        for(int l=startLineIncl;l<endLineExc;l++){
            printfl(l, false, " ");
        }
    }
    
    uint8_t GetShownLines() override
    {
        return 5;
    }
};

extern "C" void app_main(void)
{
    spilcd16::M<SPI2_HOST, PIN_LCD_MOSI, PIN_LCD_SCLK, PIN_LCD_CS, PIN_LCD_RS, GPIO_NUM_NC, PIN_LCD_BACKLIGHT, LCD135x240_90(), 8*240, 4096, 40> lcd;
    lcd.InitSpiAndGpio();
    lcd.Init_ST7789(Color::BLACK);

    auto adapter = new LineWriterToSpiLcdAdapter<32, 240, 5, 5>(&lcd);
    auto root_folder = new FolderItem("root", &root_items);
    auto m = new menu::MenuManagement(root_folder, adapter);
    m->Init();
    while (true)
    {
        button::ButtonLoop(m, PIN_BUTTON);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
