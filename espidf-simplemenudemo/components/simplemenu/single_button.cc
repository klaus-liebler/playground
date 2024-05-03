#include "single_button.hh"
#define TAG "BTN"
#include <esp_log.h>

namespace button
{
    time_t pressed_time{INT64_MAX};
    bool pressed_state{true};
    bool pressed_long_already_sent{false};
    ButtonPressResult Button(gpio_num_t gpio)
    {
        bool newstate = gpio_get_level(gpio);
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

    void ButtonLoop(menu::MenuManagement *m, gpio_num_t gpio)
    {
        ButtonPressResult r = Button(gpio);
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
    }
}