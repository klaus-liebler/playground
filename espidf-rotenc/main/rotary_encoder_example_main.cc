/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <rotenc.hh>

static const char *TAG = "example";

constexpr gpio_num_t ROT_A{GPIO_NUM_40};
constexpr gpio_num_t ROT_B{GPIO_NUM_41};
constexpr gpio_num_t ROT_C{GPIO_NUM_42};

extern "C" void app_main(){

    auto rotenc = new cRotaryEncoder(ROT_A, ROT_B, ROT_C);
    rotenc->Init();
    rotenc->Start();
    while (1) {
        int16_t rotEnc;
		bool isPressed;
		rotenc->GetValue(rotEnc, isPressed, false);
        ESP_LOGI(TAG, "Pulse count: %i %i", rotEnc, (int)isPressed);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}