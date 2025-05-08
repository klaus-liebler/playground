#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <sdkconfig.h>
#include <freertos/task.h>
#include <driver/gptimer.h>
#include <esp_rom_sys.h>
#include <esp_adc/adc_continuous.h>
#include <wifi_sta.hh>
#include <wifi_softap.hh>
#include <esp_timer.h>

#include <esp_http_server.h>

#define TAG "MAIN"

#if (SOC_ADC_DIGI_RESULT_BYTES == 2)
#define ADC_TEST_OUTPUT_TYPE    ADC_DIGI_OUTPUT_FORMAT_TYPE1
#else
#define ADC_TEST_OUTPUT_TYPE    ADC_DIGI_OUTPUT_FORMAT_TYPE2
#endif

#define ADC_TEST_FREQ_HZ        (20 * 1000)
constexpr int CONVERSIONS_IN_ONE_CALLBACK{1};

static volatile DRAM_ATTR uint32_t successfulSamples{0};
static volatile DRAM_ATTR uint32_t errors{0};
adc_continuous_handle_t handle{nullptr};
constexpr size_t SIZE{1024};
static volatile DRAM_ATTR uint32_t timestamps[SIZE];
static volatile DRAM_ATTR size_t timestampIndex{0};
static volatile DRAM_ATTR int64_t previousTime{0};


extern "C" bool IRAM_ATTR NOINLINE_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    if(timestampIndex<SIZE){
        int64_t now = esp_timer_get_time();
        timestamps[timestampIndex]=now-previousTime;
        previousTime=now;
        timestampIndex++;
    }
    if(edata->size==CONVERSIONS_IN_ONE_CALLBACK*SOC_ADC_DIGI_RESULT_BYTES){
        successfulSamples=successfulSamples+CONVERSIONS_IN_ONE_CALLBACK;
    }else{
        errors=errors+1;
    }
    return false;
}


extern "C" void app_main(void)
{

    adc_continuous_handle_cfg_t adc_config = {};

    adc_config.max_store_buf_size = 1024;
    adc_config.conv_frame_size = CONVERSIONS_IN_ONE_CALLBACK*SOC_ADC_DIGI_DATA_BYTES_PER_CONV;
    adc_continuous_new_handle(&adc_config, &handle);

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX];
    adc_pattern[0].atten = ADC_ATTEN_DB_0;
    adc_pattern[0].channel = ADC_CHANNEL_0;
    adc_pattern[0].unit = ADC_UNIT_1;
    adc_pattern[0].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
    
    adc_continuous_config_t dig_cfg = {};
    dig_cfg.sample_freq_hz = ADC_TEST_FREQ_HZ,
    dig_cfg.conv_mode = ADC_CONV_SINGLE_UNIT_1;
    dig_cfg.format = ADC_TEST_OUTPUT_TYPE;
    dig_cfg.adc_pattern = adc_pattern;
    dig_cfg.pattern_num = 1;
    adc_continuous_config(handle, &dig_cfg);

    adc_continuous_evt_cbs_t cbs = {};

    cbs.on_conv_done = s_conv_done_cb;

    adc_continuous_register_event_callbacks(handle, &cbs, nullptr);

    ESP_LOGI(TAG, "About to start ADC");
    previousTime=esp_timer_get_time();
    adc_continuous_start(handle);
    vTaskDelay(pdMS_TO_TICKS(10000));
    adc_continuous_stop(handle);
    ESP_LOGI(TAG, "About to stop ADC: successful:%lu, errors %lu", successfulSamples, errors);
    

    adc_continuous_deinit(handle);
    for(int i=0;i<SIZE;i+=8){
        printf("%lu;%lu;%lu;%lu;%lu;%lu;%lu;%lu;\n", timestamps[i+0], timestamps[i+1], timestamps[i+2], timestamps[i+3], timestamps[i+4], timestamps[i+5], timestamps[i+6], timestamps[i+7]);
    }
    
    while (true)
    {
        ESP_LOGI(TAG, "Heap %6lu", esp_get_free_heap_size());
        vTaskDelay(pdMS_TO_TICKS(10000));
    }

}
