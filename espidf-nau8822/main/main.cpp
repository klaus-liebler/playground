#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <bit>
#include <algorithm>
#include "esp32/rom/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "common-esp32.hh"


#include <nau88c22.hh>
#include <AudioPlayer.hh>



#if(CONFIG_ESP_MAIN_TASK_STACK_SIZE<8000)
    #error "ESP_MAIN_TASK_STACK_SIZE<8000" 
#endif

#include <esp_log.h>
#define TAG "MAIN"

FLASH_FILE(ready_mp3)
const uint8_t *file = ready_mp3_start;
const size_t fileLen = ready_mp3_size;

constexpr gpio_num_t PIN_I2C_SCL{GPIO_NUM_10};      // von IO27
constexpr gpio_num_t PIN_I2C_SDA{GPIO_NUM_4};       // von IO26
constexpr gpio_num_t PIN_I2S_MCLK{GPIO_NUM_14};     // von IO22
constexpr gpio_num_t PIN_I2S_BCLK{GPIO_NUM_48};      // von IO5
constexpr gpio_num_t PIN_I2S_FS{GPIO_NUM_21};       // von IO17
constexpr gpio_num_t PIN_I2S_DAC{GPIO_NUM_45}; // von IO0
constexpr gpio_num_t PIN_I2S_DATA_IN{GPIO_NUM_47};  //--
constexpr gpio_num_t SDMMC_CMD_PIN{GPIO_NUM_7};     // von IO15
constexpr gpio_num_t SDMMC_CLK_PIN{GPIO_NUM_15};    // von IO14
constexpr gpio_num_t SDMMC_D0_PIN{GPIO_NUM_16};     // von IO2
constexpr i2c_port_t I2C_PORT{I2C_NUM_0};

extern "C" void app_main()
{

    ESP_LOGI(TAG, "Starting up");
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_PORT,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority=0,
        .trans_queue_depth=0,
        .flags={
            .enable_internal_pullup=1,
        }
    };
    i2c_master_bus_handle_t i2c_master_handle{nullptr};
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_master_handle));

    nau88c22::M *codec = new nau88c22::M(i2c_master_handle, PIN_I2S_MCLK, PIN_I2S_BCLK, PIN_I2S_FS, PIN_I2S_DAC);
    AudioPlayer::Player *mp3player = new AudioPlayer::Player(codec);
    mp3player->Init();
    mp3player->PlayMP3(file, fileLen, 200, true);


   
    
    while (true)
    {
        mp3player->Loop();
    }
}