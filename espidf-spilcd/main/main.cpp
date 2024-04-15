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
#include "lvgl/lvgl.h"

#include <RGB565.hh>
#define TAG "MAIN"
using namespace SPILCD16;
using namespace menu;
using namespace display;

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

std::vector<MenuItem *> root_items = {
    new IntegerItem("Integer0", &Integer0, 0, 10, &int_cb),
    new FixedPointItem<4>("Fixed1", &Fixed1, 0, 1, &float_cb),
    new FolderItem("Folder2", &folder2_items),
    new BoolItem("Bool3", &Bool3, &bool_cb),
    new OptionItem("Option4", &option4_items, &int_cb),
    new IntegerItem("Integer5", &Integer5, 0, 10, &int_cb),
    new PlaceholderItem("Placeholder6--"),
};

enum class ButtonPressResult
{
    NO_CHANGE,
    PRESSED,
    RELEASED_SHORT,
    PRESSED_LONG,
    RELEASED_LONG,
};

time_t pressed_time{INT64_MAX};
bool pressed_state{true};
bool pressed_long_already_sent{false};

static ButtonPressResult Button()
{
    bool newstate = gpio_get_level(GPIO_NUM_0);
    time_t now = esp_timer_get_time();
    if (pressed_state && !newstate)
    { // pressed down
        pressed_state = newstate;
        pressed_time = esp_timer_get_time();
        pressed_long_already_sent = false;
        ESP_LOGD(TAG, "Pressed!");
        return ButtonPressResult::PRESSED;
    }
    else if (!pressed_state && !newstate && !pressed_long_already_sent && now - pressed_time > 400 * 1000)
    {
        pressed_state = newstate;
        pressed_long_already_sent = true;
        ESP_LOGD(TAG, "Pressed long!");
        return ButtonPressResult::PRESSED_LONG;
    }
    else if (!pressed_state && newstate)
    {
        pressed_state = newstate;
        pressed_time = INT64_MAX;
        ESP_LOGD(TAG, "Released!");
        return pressed_long_already_sent ? ButtonPressResult::RELEASED_LONG : ButtonPressResult::RELEASED_SHORT;
    }
    return ButtonPressResult::NO_CHANGE;
}

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
        frr = new FilledRectRenderer(Point2D(0,0), Point2D(240, 320), Color::YELLOW);
        ftlr= new FullTextlineRenderer<LINE_HEIGHT_PIXELS, LINE_WIDTH_PIXELS, PADDING_LEFT, PADDING_RIGHT>(&Roboto_regular);

    }
    size_t printfl(int line, bool invert, const char *format, ...) override
    {
        va_list args_list;
        va_start(args_list, format);
        line=(line+GetAvailableLines())%GetAvailableLines();
        size_t ret = ftlr->printfl(line, invert?Color::BLACK:Color::YELLOW, invert?Color::YELLOW:Color::BLACK, format, args_list);
        va_end(args_list);
        host->DrawAsyncAsSync(ftlr, true);
        return ret;
        
    }
    void ClearScreenAndResetStartline(bool invert = false, uint8_t start_textline_nominator = 0, uint8_t start_textline_denominator = 1) override
    {
        host->prepareVerticalStrolling(0, 0);
        host->doVerticalStrolling(0);
        startline_px=0;
        frr->Reset();
        host->DrawAsyncAsSync(frr, false);
    }
    void SetStartline(uint8_t startline) override//TODO check whether this function is really needed
    {
        ESP_LOGE(TAG, "SetStartline not supported");
    }
    void Scroll(int textLines) override{
        int step_px = 2;
        int count = 12;
        ESP_LOGD(TAG, "Scoll count=%d, step_px=%d, old startline_px=%d", count, step_px, startline_px);
        for (int i = 0; i < count; i++)
        {
            startline_px += step_px;
            startline_px = (startline_px + 240) % 240;
            host->doVerticalStrolling(startline_px);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        ESP_LOGI(TAG, "New startline_px=%d", startline_px);
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
    // M(spi_host_device_t spiHost, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl)
    SPILCD16::M<SPI2_HOST, GPIO_NUM_7, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_NC, GPIO_NUM_18, LCD135x240(12)> lcd;
    lcd.InitSpiAndGpio();
    lcd.Init_ST7789(Color::YELLOW);
    //auto r = new FullTextlineRenderer<24, 135, 5, 5>(&Roboto_regular);
    //r->printfl(0, Color::BLACK, Color::GREEN, "Top Line" LV_SYMBOL_OK "123");
    //lcd.DrawAsyncAsSync(r, true);

    auto adapter = new LineWriterToSpiLcsAdapter<24, 135,5,5>(&lcd, 24);

    auto root_folder = new FolderItem("root", &root_items);
    auto m = new menu::MenuManagement(root_folder, adapter);
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

    uint16_t topline = 0;
    while (true)
    {
        if (lcd.doVerticalStrolling(topline) != ErrorCode::OK)
        {
            ESP_LOGE(TAG, "lcd.doVerticalStrolling(topline)!=ErrorCode::OK");
        }
        topline += 1;
        topline = topline % 192;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
