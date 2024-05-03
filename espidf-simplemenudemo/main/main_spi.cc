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
#include "simple_menu.hh"
#include "single_button.hh"
#include <lcd_font.hh>

#include <RGB565.hh>
#define TAG "MAIN"
using namespace spilcd16;
using namespace menu;
using namespace display;

#include "handler_and_menu_definition.inc"

template <uint8_t LINE_HEIGHT_PIXELS, uint8_t LINE_WIDTH_PIXELS, uint8_t PADDING_LEFT, uint8_t PADDING_RIGHT>
class LineWriterToSpiLcsAdapter : public FullLineWriter
{
private:
    IRendererHost *host;
    uint8_t lineHeight;
    FilledRectRenderer* frr;
    FullTextlineRenderer<LINE_HEIGHT_PIXELS, LINE_WIDTH_PIXELS, PADDING_LEFT, PADDING_RIGHT>* ftlr;
    uint8_t startline_px{0};

public:
    LineWriterToSpiLcsAdapter(IRendererHost *host, uint8_t lineHeight) : host(host), lineHeight(lineHeight) {
        frr = new FilledRectRenderer(Point2D(0,0), Point2D(240, 320), Color::BLACK);
        ftlr= new FullTextlineRenderer<LINE_HEIGHT_PIXELS, LINE_WIDTH_PIXELS, PADDING_LEFT, PADDING_RIGHT>(&arial_and_symbols_16px1bpp::font);
    }
    
    size_t printfl(int line, bool invert, const char *format, ...) override
    {
        va_list args_list;
        va_start(args_list, format);
        line=(line+GetAvailableLines())%GetAvailableLines();
        size_t ret = ftlr->printfl(line, invert?Color::BLACK:Color::YELLOW, invert?Color::YELLOW:Color::BLACK, format, args_list);
        va_end(args_list);
        host->Draw(ftlr, true);
        return ret; 
    }
    
    void ClearScreenAndResetStartline(bool invert = false, uint8_t start_textline_nominator = 0, uint8_t start_textline_denominator = 1) override
    {
        host->prepareVerticalStrolling(0, 0);
        host->doVerticalStrolling(0);
        startline_px=0;
        frr->Reset();
        host->Draw(frr, false);
    }
    
    void SetStartline(uint8_t startline) override//TODO check whether this function is really needed
    {
        ESP_LOGE(TAG, "SetStartline not supported");
    }
    
    void Scroll(int textLines) override{
        int step_px = 4;
        int count = 6;
        ESP_LOGD(TAG, "Scoll count=%d, step_px=%d, old startline_px=%d", count, step_px, startline_px);
        for (int i = 0; i < count; i++)
        {
            startline_px += step_px;
            startline_px = (startline_px + 240) % 240;
            host->doVerticalStrolling(startline_px);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
        ESP_LOGD(TAG, "New startline_px=%d", startline_px);
    }
    
    uint8_t GetShownLines() override
    {
        return 10;
    }
    
    uint8_t GetAvailableLines() override
    {
        return 10;
    }
};

extern "C" void app_main(void)
{
    spilcd16::M<SPI2_HOST, GPIO_NUM_7, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_NC, GPIO_NUM_18, LCD135x240(12)> lcd;
    lcd.InitSpiAndGpio();
    lcd.Init_ST7789(Color::BLACK);

    auto adapter = new LineWriterToSpiLcsAdapter<24, 135, 5, 5>(&lcd, 24);
    auto root_folder = new FolderItem("root", &root_items);
    auto m = new menu::MenuManagement(root_folder, adapter);
    m->Init();
    while (true)
    {
        button::ButtonLoop(m, GPIO_NUM_0);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
