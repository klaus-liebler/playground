#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include "esp_bt.h"
#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "esp_timer.h"
//#include "MP3DecoderHelix.h"
#include "MP3HeaderDecoder.hh"
#include "Alarm.mp3.h"

#include "tas5806m.hh"
#define TAG "MAIN"

constexpr i2s_port_t I2S_NUM = I2S_NUM_0;


/* event for handler "bt_av_hdl_stack_up */
enum
{
    BT_APP_EVT_STACK_UP = 0,
};

/* handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param);


void dataCallback(MP3FrameInfo &info, int16_t *pwm_buffer, size_t len) {
    //ESP_LOGI(TAG, "Callback channels %d samples %d len %d", info.nChans, info.outputSamps, len); //immer Callback channels 2 samples 2304 len 2304
    size_t i2s_bytes_write;
    i2s_write(I2S_NUM, pwm_buffer, len*2, &i2s_bytes_write, portMAX_DELAY);
}


MP3HeaderDecoder mp3(I2S_NUM);
extern "C" void app_main(void)
{
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    

    mp3.Init(GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_4);

    I2C i2c_bus(I2C_NUM_0);
    i2c_bus.Init(GPIO_NUM_41, GPIO_NUM_42);
    i2c_bus.SearchBus();

    TAS5806::M tas5806_manager(i2c_bus, TAS5806::ADDR7bit::DVDD_4k7, GPIO_NUM_39);
    ESP_ERROR_CHECK(tas5806_manager.Init(60));


    int64_t lastHupe = esp_timer_get_time();
    while (true)
    {
        mp3.Loop();
        int64_t now = esp_timer_get_time();
        if(now-lastHupe>2000000){
            mp3.Play(Alarm_hupe_mp3, sizeof(Alarm_hupe_mp3));
            ESP_LOGI(TAG, "Played Hupe");
            lastHupe=now;
        }
    }
    


    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    bt_cfg.bluetooth_mode=ESP_BT_MODE_CLASSIC_BT;

    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_bluedroid_init()) != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_bluedroid_enable()) != ESP_OK)
    {
        ESP_LOGE(BT_AV_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    /* create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use fixed pin code
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '1';
    pin_code[1] = '2';
    pin_code[2] = '3';
    pin_code[3] = '4';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    
    while(true){
        vTaskDelay(10);
    }
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_AUTH_CMPL_EVT:
    {
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(BT_AV_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(BT_AV_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        }
        else
        {
            ESP_LOGE(BT_AV_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %lu", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%lu", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    default:
    {
        ESP_LOGI(BT_AV_TAG, "event: %d", event);
        break;
    }
    }
    return;
}
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    switch (event)
    {
    case BT_APP_EVT_STACK_UP:
    {
        /* set up device name */
        const char *dev_name = "ESP_SPEAKER";
        esp_bt_dev_set_device_name(dev_name);

        esp_bt_gap_register_callback(bt_app_gap_cb);

        /* initialize AVRCP controller */
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
        /* initialize AVRCP target */
        assert(esp_avrc_tg_init() == ESP_OK);
        esp_avrc_tg_register_callback(bt_app_rc_tg_cb);

        esp_avrc_rn_evt_cap_mask_t evt_set = {0};
        esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
        assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

        /* initialize A2DP sink */
        esp_a2d_register_callback(&bt_app_a2d_cb);
        esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
        esp_a2d_sink_init();

        /* set discoverable and connectable mode, wait to be connected */
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        break;
    }
    default:
        ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
        break;
    }
}
