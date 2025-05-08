#include <stdio.h>
#include <algorithm>
#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_spi_flash.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <sys/param.h>
#include <nvs_flash.h>
#include <esp_netif.h>
#include "esp_log.h"

#include <driver/gpio.h>
#include <driver/mcpwm.h>

#include <pid_t1_controller.hh>

static const char *TAG = "main";

#include <owb.h>
#include <owb_rmt.h>
#include <ds18b20.h>

constexpr int FREQUENCY_HEATER{1};
constexpr gpio_num_t PIN_HEATER_OR_LED_POWER{GPIO_NUM_12};
constexpr gpio_num_t PIN_ONEWIRE{GPIO_NUM_14};

constexpr rmt_channel_t CHANNEL_ONEWIRE_TX{RMT_CHANNEL_1};
constexpr rmt_channel_t CHANNEL_ONEWIRE_RX{RMT_CHANNEL_2};

constexpr mcpwm_timer_t MCPWM_TIMER_HEATER_OR_LED_POWER{MCPWM_TIMER_2};
constexpr mcpwm_io_signals_t MCPWM_IO_HEATER_OR_LED_POWER{MCPWM2A};
constexpr mcpwm_generator_t MCPWM_GEN_HEATER_OR_LED_POWER{MCPWM_GEN_A};

constexpr int64_t oneWireReadoutIntervalMs{1000};

constexpr float T_CYC{1};
constexpr float KPR{17.5};
constexpr float TN_SECS{40};
constexpr float TV_SECS{8.4};

//a1=-1,67,a2=0,67, b0=43,4, b1=-83,31 und b2=39,96

//constexpr float T_CYC{0.05};
//constexpr float KPR{10};
//constexpr float TN_SECS{4};
//constexpr float TV_SECS{0.5};

extern "C" void app_main();

int64_t GetMillis64()
{
    return esp_timer_get_time() / 1000ULL;
}

void app_main(void)
{
    int64_t nextOneWireReadout{INT64_MAX};
    float heaterTemperatureDegCel{0.0};
    float setpointHeater{0};
    float setpointTemperature{50};

    ESP_ERROR_CHECK(mcpwm_group_set_resolution(MCPWM_UNIT_0, 1000000));
    ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_IO_HEATER_OR_LED_POWER, PIN_HEATER_OR_LED_POWER));
    mcpwm_config_t pwm_config;
    pwm_config.cmpr_a = 0;     //duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;     //duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    pwm_config.frequency = FREQUENCY_HEATER;
    ESP_ERROR_CHECK(mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_HEATER_OR_LED_POWER, &pwm_config));


    DS18B20_Info *ds18b20_info = NULL;
    owb_rmt_driver_info rmt_driver_info;
    OneWireBus *owb = owb_rmt_initialize(&rmt_driver_info, PIN_ONEWIRE, CHANNEL_ONEWIRE_TX, CHANNEL_ONEWIRE_RX);
    owb_use_crc(owb, true); // enable CRC check for ROM code
    // Find all connected devices
    OneWireBus_ROMCode rom_code;
    owb_status status = owb_read_rom(owb, &rom_code);
    if (status == OWB_STATUS_OK)
    {
        ds18b20_info = ds18b20_malloc();      // heap allocation
        ds18b20_init_solo(ds18b20_info, owb); // only one device on bus
        ds18b20_use_crc(ds18b20_info, true);  // enable CRC check on all reads
        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION_12_BIT);
        ds18b20_convert_all(owb);
        nextOneWireReadout = GetMillis64() + oneWireReadoutIntervalMs;
        ESP_LOGI(TAG, "OneWire: DS18B20 successfully initialized.");
    }
    else
    {
        ESP_LOGE(TAG, "OneWire: An error occurred reading ROM code: %d", status);
    }

    PID_T1::Controller<float> *heaterPIDController = new PID_T1::Controller<float>(&heaterTemperatureDegCel, &setpointHeater, &setpointTemperature, 0, 100, PID_T1::Mode::OFF, PID_T1::AntiWindup::ON_SWICH_OFF_INTEGRATOR, PID_T1::Direction::DIRECT, T_CYC*1000.0);
    ErrorCode ec = heaterPIDController->SetKpTnTv(KPR, TN_SECS * 1000, TV_SECS * 1000, TV_SECS * 200);
    if(ec != ErrorCode::OK){
        ESP_LOGW(TAG, "heaterPIDController->SetKpTnTv produced ErrorCode %d", (int)ec);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    heaterPIDController->SetMode(PID_T1::Mode::CLOSEDLOOP, GetMillis64());
    while (true)
    {
        if (GetMillis64() > nextOneWireReadout)
        {
            ds18b20_read_temp(ds18b20_info, &heaterTemperatureDegCel);
            ds18b20_convert_all(owb);
            nextOneWireReadout = GetMillis64() + oneWireReadoutIntervalMs;
            ESP_LOGI(TAG, "Got new temperature %F", heaterTemperatureDegCel);
        }

        if (heaterPIDController->Compute(GetMillis64()) == ErrorCode::OK)
        { // OK means: Value changed
            ESP_LOGI(TAG, "Computed a new  setpointHeater %F", setpointHeater);
            mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_HEATER_OR_LED_POWER, MCPWM_GEN_HEATER_OR_LED_POWER, setpointHeater);
        }
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
