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
#include <u8g2.h>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "ssd1306_fonts.hh"

constexpr gpio_num_t SCL{GPIO_NUM_6};
constexpr gpio_num_t SDA{GPIO_NUM_7};
i2c_master_bus_handle_t bus_handle{nullptr};
i2c_master_dev_handle_t dev_handle{nullptr};


enum class MenuItemResult{
    NO_ACTION,
    REDRAW_SECOND_LINE,
    OPEN_NEW_FULLSCREEN,
    CLOSE_MYSELF,
    REDRAW,
};
enum class OnOpenFullscreenResult{
    OK,
    NOT_OK,//for placeholder
    NOT_OK_AND_CLOSE_PREVIOUS_WINDOW,//for ReturnItem
};

class MenuItem
{
protected:
    const char *name;

public:
    MenuItem(const char *name) : name(name) {}
    virtual void RenderCompact(FullLineWriter *lw, int page_modulo_inside, bool invert) = 0;
    virtual void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t textLines){};
    virtual OnOpenFullscreenResult OnOpenFullscreen(){return OnOpenFullscreenResult::OK;}//This is the normal case
    virtual void OnCloseFullscreen(){return;}//This is the normal case
    virtual MenuItemResult Up(){return MenuItemResult::CLOSE_MYSELF;};
    virtual MenuItemResult Down(){return MenuItemResult::CLOSE_MYSELF;};
    virtual MenuItemResult Select(MenuItem** itemToOpen){return MenuItemResult::CLOSE_MYSELF;};
    virtual MenuItemResult Back(){return MenuItemResult::CLOSE_MYSELF;};
    const char* GetName(){return name;}
};

class IntegerItem : public MenuItem
{
private:
    int value, value_min, value_max;

public:
    IntegerItem(const char *name, int value_init, int value_min, int value_max) : MenuItem(name), value(value_init), value_min(value_min), value_max(value_max) {}
    void RenderCompact(FullLineWriter *lw, int line, bool invert) override
    {
        lw->printfl(line, invert, "%s\t%d", name, value);
    }

    void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t screenHeight) override
    {
        if(initial){
            lw->ClearScreenAndResetStartline();
            lw->printfl(0, false, "Edit Value");
        }
        lw->printfl(1, true, "%d", value);
    }
    MenuItemResult Up() override
    {
        value++;
        if(value>value_max) value=value_min;
        return MenuItemResult::REDRAW;
    }
    MenuItemResult Down() override
    {
        value--;
        if(value<value_min) value=value_max;
        return MenuItemResult::REDRAW;
    }
};

class BoolItem : public MenuItem
{
private:
    bool value;

public:
    BoolItem(const char *name, bool value_init) : MenuItem(name), value(value_init) {}
    void RenderCompact(FullLineWriter *lw, int line, bool invert) override
    { 
        if(value){
            lw->printfl(line, invert, "%s\t\x1b\x10", name);
        }else{
            lw->printfl(line, invert, "%s\t\x1b\xf", name);
        }
    }

    void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t screenHeight) override
    {
        if(initial){
            lw->ClearScreenAndResetStartline();
            lw->printfl(0, false, "Edit Value");
        }
        if(value){
            lw->printfl(1, true, "\x1b\x10");
        }
        else{
            lw->printfl(1, true, "\x1b\xf");
        }
    }

    MenuItemResult Up() override
    {
        value = !value;
        return MenuItemResult::REDRAW;
    }
    MenuItemResult Down() override
    {
        value = !value;
        return MenuItemResult::REDRAW;
    }
};

template <uint16_t UNITS_PER_INTEGER = 10>
class FixedPointItem : public MenuItem
{
private:
    int value, value_min, value_max;

public:
    FixedPointItem(const char *name, float value_init, float value_min, float value_max) : MenuItem(name), value(value_init * UNITS_PER_INTEGER), value_min(value_min * UNITS_PER_INTEGER), value_max(value_max * UNITS_PER_INTEGER) {}
    void RenderCompact(FullLineWriter *lw, int line, bool invert) override
    {
        lw->printfl(line, invert, "%s\t%.2f", name, (float)value / (float)UNITS_PER_INTEGER);
    }

    void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t screenHeight) override
    {
        if(initial){
            lw->ClearScreenAndResetStartline();
            lw->printfl(0, false, "Edit Value");
        }
        lw->printfl(1, true, "%.2f", (float)value / (float)UNITS_PER_INTEGER);
    }

    MenuItemResult Up() override
    {
        value++;
        if(value>value_max) value=value_min;
        return MenuItemResult::REDRAW;
    }
    MenuItemResult Down() override
    {
        value--;
        if(value<value_min) value=value_max;
        return MenuItemResult::REDRAW;
    }
};

class ReturnItem : public MenuItem
{
private:
public:
    ReturnItem() : MenuItem("\x1b\x2\x1b\x2\x1b\x2\x1b\x2") {}
    void RenderCompact(FullLineWriter *lw, int line, bool invert) override
    {
        lw->printfl(line, invert, "%s", name);
    }

    OnOpenFullscreenResult OnOpenFullscreen() override{return OnOpenFullscreenResult::NOT_OK_AND_CLOSE_PREVIOUS_WINDOW;}
};

class PlaceholderItem:public MenuItem{
    public:
    PlaceholderItem(const char* name) : MenuItem(name) {}
    void RenderCompact(FullLineWriter *lw, int line, bool invert) override
    {
        lw->printfl(line, invert, "%s", name);
    }
    OnOpenFullscreenResult OnOpenFullscreen() override{return OnOpenFullscreenResult::NOT_OK;}
};

class FolderItem : public MenuItem
{
private:
    std::vector<MenuItem *> *content;
    size_t selected_menu{0}; //index in the content vector
    size_t selected_line{0}; //index in the graphics ram o
    int movement{0};
public:
    FolderItem(const char *name, std::vector<MenuItem *> *content) : MenuItem(name), content(content) {}
    void RenderCompact(FullLineWriter *lw, int page, bool invert) override
    {
        lw->printfl(page, invert, "%s\t\x1b\x3", name);
    }

    MenuItem * GetContent(int uncorrected_index){
        return content->at(ssd1306::modulo(uncorrected_index, content->size()));
    }

    void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t textLines) override{
        if(initial){
            ESP_LOGI(TAG, "Menu Full update");
            lw->ClearScreenAndResetStartline(false,0);
            selected_line=1;
            int menu_idx = selected_menu-1;
            for (int textline = 0; textline < textLines; textline++)
            {
                int corr_menu_idx = ssd1306::modulo(menu_idx, content->size());
                MenuItem* itm = content->at(corr_menu_idx);
                itm->RenderCompact(lw, textline, menu_idx==selected_menu);
                menu_idx++;
            }
        }
        else if(movement==+1){
            ESP_LOGD(TAG, "Update ram %d with %s and %d with %s and %d with %s",
                ssd1306::modulo(selected_line, textLines),     GetContent(selected_menu)->GetName(), 
                ssd1306::modulo(selected_line + 1, textLines), GetContent(selected_menu+1)->GetName(), 
                ssd1306::modulo(selected_line + (textLines-1), textLines), GetContent(selected_menu+5)->GetName());
            lw->printfl(selected_line-1, false, " ");//delete first line on display (needs at least a space character; otherwise nothiong will be printed)
            GetContent(selected_menu)->RenderCompact(lw, selected_line, false);//redraw selected line as unselected
            GetContent(selected_menu+1)->RenderCompact(lw, selected_line+1, true); //redraw next line as selected
            lw->Scroll(+1);
            GetContent(selected_menu+textLines-1)->RenderCompact(lw, selected_line+textLines-1, false);
            selected_line = ssd1306::modulo(++selected_line, textLines);
            selected_menu =ssd1306::modulo(++selected_menu, content->size());
            movement=0;
        }
    }


    void RenderFullScreen32(FullLineWriter *lw, bool initial, uint8_t textLines)
    {
        if(initial){
            ESP_LOGI(TAG, "Menu Full update");
            lw->ClearScreenAndResetStartline(false, 16);
            selected_line=3;
            int menu_idx = selected_menu-3;
            for (int page = 0; page < 8; page++)
            {
                int corr_menu_idx = ssd1306::modulo(menu_idx, content->size());
                MenuItem* itm = content->at(corr_menu_idx);
                itm->RenderCompact(lw, page, menu_idx==selected_menu);
                menu_idx++;
            }
        }
        else if(movement==+1){
            ESP_LOGD(TAG, "Update ram %d with %s and %d with %s and %d with %s",
                ssd1306::modulo(selected_line, textLines),     GetContent(selected_menu)->GetName(), 
                ssd1306::modulo(selected_line + 1, textLines), GetContent(selected_menu+1)->GetName(), 
                ssd1306::modulo(selected_line + textLines-1, textLines), GetContent(selected_menu+5)->GetName());
            GetContent(selected_menu)->RenderCompact(lw, selected_line, false);
            GetContent(selected_menu+1)->RenderCompact(lw, selected_line+1, true);
            lw->Scroll(1);
            GetContent(selected_menu+5)->RenderCompact(lw, selected_line+5, false);
            selected_line = ssd1306::modulo(++selected_line, textLines);
            selected_menu =ssd1306::modulo(++selected_menu, content->size());
            movement=0;
        }

        
    }

    MenuItemResult Up() override
    {
       return MenuItemResult::REDRAW;
    }
    MenuItemResult Down() override
    {
       movement=+1;
       return MenuItemResult::REDRAW;
    }

    MenuItemResult Select(MenuItem** itemToOpen) override{
        *itemToOpen=content->at(selected_menu);
        return MenuItemResult::OPEN_NEW_FULLSCREEN;
    }
};

class OptionItem : public MenuItem
{
private:
    std::vector<const char *> *options;
    size_t selected_option{0}; //index in the content vector
    size_t selected_line{0}; //index in the graphics ram o
    int movement{0};
public:
    OptionItem(const char *name, std::vector<const char *> *options) : MenuItem(name), options(options) {}
    void RenderCompact(FullLineWriter *lw, int page, bool invert) override
    {
        lw->printfl(page, invert, "%s\t%s", name, options[selected_option]);
    }

    const char * GetSelectedOptionName(int offset=0){
        return options->at(ssd1306::modulo(selected_option+offset, options->size()));
    }

    size_t GetSelectedOptionIndex(){
        return selected_option;
    }

    void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t textLines) override{
        if(initial){
            ESP_LOGI(TAG, "Menu Full update");
            lw->ClearScreenAndResetStartline(false,0);
            selected_line=1;
            int option_idx = selected_option-1;
            for (int textline = 0; textline < textLines; textline++)
            {
                int corr_option_idx = ssd1306::modulo(option_idx, options->size());
                const char* itm = options->at(corr_option_idx);
                if(option_idx==selected_option){
                    lw->printfl(textline, true, "\x1b\xe %s", itm);
                }else{
                    lw->printfl(textline, false, "   %s", itm);
                }
                option_idx++;
            }
        }
        else if(movement==+1){
            lw->printfl(selected_line-1, false, " ");//delete first line on display (needs at least a space character; otherwise nothiong will be printed)
            lw->printfl(selected_line, false, "   %s", GetSelectedOptionName());//redraw selected line as unselected
            lw->printfl(selected_line+1, true, "\x1b\xe %s", GetSelectedOptionName(+1));//redraw next line as selected 
            lw->Scroll(+1);
            lw->printfl(selected_line+textLines-1, false, "   %s", GetSelectedOptionName(textLines-1));
            selected_line = ssd1306::modulo(++selected_line, textLines);
            selected_option =ssd1306::modulo(++selected_option, options->size());
            movement=0;
        }
    }

    MenuItemResult Up() override
    {
       return MenuItemResult::REDRAW;
    }
    MenuItemResult Down() override
    {
       movement=+1;
       return MenuItemResult::REDRAW;
    }

};



// Management enthält ein Vector mit dem Pfad der Bearbeitung
// MenuFolder enthält andere MenuFolder und auch MenuItems
// Wird ein MenuItem selektiert, wird es dem Pfad hinzugefügt und es darf sich FullScreen zeigen
// Normales Item wechselt es in eine Bearbeitungssicht Show. es bekommt dann alle Nutzerbefehle Up, Down, Select, Back


class MenuManagement
{
private:
    FolderItem* root;
    FullLineWriter* lw;
    std::vector<MenuItem *> path;
    uint8_t textLines{4};
public:
    MenuManagement(FolderItem* root, FullLineWriter* lw):root(root), lw(lw){}

    void Init(){
        root->RenderFullScreen(lw, true, this->textLines);
        path.push_back(this->root);
    }

    void Down(){
        MenuItem* mi = path.back();
        switch (mi->Down())
        {
        case MenuItemResult::REDRAW:
            mi->RenderFullScreen(lw, false, this->textLines);
            break;
        case MenuItemResult::NO_ACTION:
            break;
        default:
            break;
        }
    }

    void GoBack(){
        MenuItem* mi = path.back();
        mi->OnCloseFullscreen();
        path.pop_back();
        path.back()->OnOpenFullscreen();
        path.back()->RenderFullScreen(lw, true, this->textLines);
    }
    void Select(){
        MenuItem *toOpen{nullptr};
        MenuItem* mi = path.back();
        switch (mi->Select(&toOpen))
        {
        case MenuItemResult::NO_ACTION:
            break;
        case MenuItemResult::REDRAW:
            mi->RenderFullScreen(lw, false, this->textLines);
            break;
        case MenuItemResult::CLOSE_MYSELF:
            GoBack();
            break;
        case MenuItemResult::OPEN_NEW_FULLSCREEN:{
            assert(toOpen);
            switch (toOpen->OnOpenFullscreen())
            {
            case OnOpenFullscreenResult::OK:
                mi->OnCloseFullscreen();
                toOpen->RenderFullScreen(lw, true, this->textLines);
                path.push_back(toOpen);
                break;
            case OnOpenFullscreenResult::NOT_OK_AND_CLOSE_PREVIOUS_WINDOW:
                GoBack();
                break;
            case OnOpenFullscreenResult::NOT_OK:
                break;
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
        
    }
};

ssd1306::M<128, 64> *lcd{nullptr};

std::vector<MenuItem *> folder2_1_items = {
    new IntegerItem("Integer2_1_0", 1, 0, 10),
    new ReturnItem(),
};

std::vector<MenuItem *> folder2_items = {
    new IntegerItem("Integer2_0", 1, 0, 10),
    new FolderItem("Folder2_1", &folder2_1_items),
    new IntegerItem("Integer2_2", 2, 0, 10),
    new IntegerItem("Integer2_3", 2, 0, 10),
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
    new IntegerItem("Integer0", 1, 0, 10),
    new FixedPointItem<4>("Fixed1", 0.25, 0, 1),
    new FolderItem("Folder2", &folder2_items),
    new BoolItem("Bool3", false),
    new OptionItem("Option", &option_items),
    new IntegerItem("Integer5", 4, 0, 10),
    new IntegerItem("Integer6", 4, 0, 10),
    new IntegerItem("Integer7", 4, 0, 10),
    new IntegerItem("Integer8", 4, 0, 10),
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

    lcd = new ssd1306::M<128,64>(&font::Font16x8);
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