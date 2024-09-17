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
#include "driver/i2c.h"
#include "common-esp32.hh"

#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_NONSTANDARD_BUT_LOGICAL
#define MINIMP3_IMPLEMENTATION

#if(CONFIG_ESP_MAIN_TASK_STACK_SIZE<8000)
    #error "ESP_MAIN_TASK_STACK_SIZE<8000" 
#endif

#include "minimp3.h"

#include <esp_log.h>
#define TAG "MAIN"

FLASH_FILE(ready_mp3)
const uint8_t *file = ready_mp3_start;
const size_t fileLen = ready_mp3_size;

constexpr gpio_num_t I2C_SCL_PIN{GPIO_NUM_10};      // von IO27
constexpr gpio_num_t I2C_SDA_PIN{GPIO_NUM_4};       // von IO26
constexpr gpio_num_t I2S_MCLK_PIN{GPIO_NUM_14};     // von IO22
constexpr gpio_num_t I2S_BLK_PIN{GPIO_NUM_48};      // von IO5
constexpr gpio_num_t I2S_WS_PIN{GPIO_NUM_21};       // von IO17
constexpr gpio_num_t I2S_DATA_OUT_PIN{GPIO_NUM_45}; // von IO0
constexpr gpio_num_t I2S_DATA_IN_PIN{GPIO_NUM_47};  //--
constexpr gpio_num_t SDMMC_CMD_PIN{GPIO_NUM_7};     // von IO15
constexpr gpio_num_t SDMMC_CLK_PIN{GPIO_NUM_15};    // von IO14
constexpr gpio_num_t SDMMC_D0_PIN{GPIO_NUM_16};     // von IO2
constexpr uint8_t NAU8822_I2C_ADDRESS{0X1A};
constexpr size_t AUDIO_BUFFER_SIZE_IN_SAMPLES{2048};
constexpr size_t TARGET_CHANNELS{2};
namespace MP3
{
    constexpr size_t SAMPLES_PER_FRAME = 1152;
    constexpr size_t FRAME_MAX_SIZE_BYTES = 1024;
    constexpr size_t HEADER_LENGTH_BYTES = 4;
    constexpr uint8_t SYNCWORDH = 0xff;
    constexpr uint8_t SYNCWORDL = 0xf0;
}

i2s_chan_handle_t tx_handle;
mp3dec_t *decoder;
int32_t frameStart{0};
int16_t *outBuffer{nullptr};

esp_err_t i2sMasterInit(uint32_t sampleRate, i2s_data_bit_width_t bitsPerSample)
{

    // setup a standard config and the channel
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle, NULL));

    // setup the i2s config
    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = sampleRate,
            .clk_src = I2S_CLK_SRC_DEFAULT,
#if defined(CONFIG_IDF_TARGET_ESP32S3)
            .ext_clk_freq_hz = 0,
#endif
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bitsPerSample, I2S_SLOT_MODE_STEREO), // the wav file bit and channel config
        .gpio_cfg = {
            // refer to configuration.h for pin setup
            .mclk = I2S_MCLK_PIN,
            .bclk = I2S_BLK_PIN,
            .ws = I2S_WS_PIN,
            .dout = I2S_DATA_OUT_PIN,
            .din = I2S_DATA_IN_PIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    return i2s_channel_init_std_mode(tx_handle, &std_cfg);
}

esp_err_t i2cWriteNAU8822(uint8_t addr, int16_t data)
{
    uint8_t write_buf[2] = {(uint8_t)((addr << 1) | (data >> 8)), (uint8_t)((data & 0x00ff))};
    return i2c_master_write_to_device(I2C_NUM_0, NAU8822_I2C_ADDRESS, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

esp_err_t i2cReadNAU8822(uint8_t addr, uint16_t &reg_data)
{
    uint8_t write_buf[1] = {(uint8_t)((addr << 1))};
    uint8_t read_buf[2];
    esp_err_t espRc = i2c_master_write_read_device(I2C_NUM_0, NAU8822_I2C_ADDRESS, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf), pdMS_TO_TICKS(1000));
    reg_data = read_buf[1] + (read_buf[0] << 8);
    return espRc;
}

esp_err_t i2cCheckNAU8822()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (NAU8822_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NAU8822 does not respond");
        return ESP_FAIL;
    }
    uint16_t data{0};
    ESP_ERROR_CHECK(i2cReadNAU8822(0x3F, data));
    if (data != 0x1A)
    {
        ESP_LOGE(TAG, "NAU8822 does respond the wrong id %d", data);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Found NAU8822!");
    return ESP_OK;
}

// 0 does not mean "mute", but minimal volume
void setSpeakersVolume(uint8_t volume0_255)
{
    uint16_t value = volume0_255 >> 2;
    ESP_LOGI(TAG, "Setting Speaker Volume to %ddB", value);
    i2cWriteNAU8822(54, value);
    i2cWriteNAU8822(55, 0x100 | value);
}

void muteSpeakers(void)
{
    ESP_LOGI(TAG, "Mute Speakers");
    i2cWriteNAU8822(54, 0x040);
    i2cWriteNAU8822(55, 0x140);
}

void setHeadphonesVolume(uint8_t volume0_255)
{
    uint16_t value = volume0_255 >> 2;
    ESP_LOGI(TAG, "Setting Headphones Volume to %ddB", value);
    i2cWriteNAU8822(52, value);
    i2cWriteNAU8822(53, 0x100 | value);
}

void muteHeadphones(void)
{
    ESP_LOGI(TAG, "Mute Headphones");
    i2cWriteNAU8822(52, 0x040);
    i2cWriteNAU8822(53, 0x140);
}

void i2cSetupNAU8822Play(uint8_t initialVolume_0_255 = 255) // this is the hardware default volume
{

    ESP_ERROR_CHECK(i2cWriteNAU8822(0, 0x000)); // Software Reset
    vTaskDelay(pdMS_TO_TICKS(100));
    muteSpeakers(); // according to datasheet power up procedure
    muteHeadphones();
    i2cWriteNAU8822(1, 0b1'1111'1101); // Enable Everything
    vTaskDelay(pdMS_TO_TICKS(250));    // according to datasheet power up procedure
    i2cWriteNAU8822(2, 0b1'1000'0000); // Power 2: Enable Headphones
    i2cWriteNAU8822(3, 0b1'1110'1111); // Power 3: Enable everything
    i2cWriteNAU8822(4, 0b0'0001'0000); // Audio Interface: normal phase, 16bit (instead of default 24), standard I2S format
    i2cWriteNAU8822(6, 0b0'0000'1100); // MCLK, pin#11 used as master clock, divided by 1, divide by 8, slave mode

    i2cWriteNAU8822(43, 0b0'0001'0000); // Right Speaker Submixer -->BTL-Configuration
    i2cWriteNAU8822(49, 0b0'0101'1110); // Output Control: Left DAC output to RMIX!. Außerdem: Thermal shutdown enable, Speaker für 5V Versorgungsspannung optimiert.
    i2cWriteNAU8822(50, 0x1DD);         // I wanna have it loud!
    // i2cWriteNAU8822(51, 0x001);

    setSpeakersVolume(initialVolume_0_255);
    setHeadphonesVolume(initialVolume_0_255);
}

void i2cInit()
{
    i2c_port_t i2c_master_port = I2C_NUM_0;
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA_PIN;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_SCL_PIN;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void makeStereo(int16_t *read_buf, size_t samplesAvailable)
{
    for (size_t i = 0; i < samplesAvailable; i++)
    {
        read_buf[2 * samplesAvailable - 1 - 2 * i] = read_buf[2 * samplesAvailable - 2 - 2 * i] = read_buf[samplesAvailable - 1 - i];
    }
}

static int FindSyncWordOnAnyPosition(const uint8_t *file, size_t fileLen, int offset)
{
    for (int i = offset; i < fileLen - 1; i++)
    {
        if ((file[i + 0] & MP3::SYNCWORDH) == MP3::SYNCWORDH && (file[i + 1] & MP3::SYNCWORDL) == MP3::SYNCWORDL)
            return i;
    }
    return -1;
}

extern "C" void app_main()
{

    ESP_LOGI(TAG, "Starting up");
    i2cInit();


    
    i2cCheckNAU8822();
    i2cSetupNAU8822Play();
    ESP_ERROR_CHECK(i2sMasterInit(24000, I2S_DATA_BIT_WIDTH_16BIT));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_handle));

    outBuffer = new int16_t[2 * MP3::SAMPLES_PER_FRAME];
    decoder = new mp3dec_t();
    mp3dec_init(decoder);
    int frameStart = FindSyncWordOnAnyPosition(file, fileLen, 0);
    if (frameStart < 0)
    {
        ESP_LOGE(TAG, "No synch word found in file!");
        return;
    }
    else{
        ESP_LOGI(TAG, "First Frame starts at offset %d", frameStart);
    }

    while (true)
    {
        if (frameStart >= fileLen)
        {
            ESP_LOGI(TAG, "Reached End of MP3 File.");
            break;
        }
        int bytesLeft = fileLen - frameStart;
        mp3dec_frame_info_t info = {};
        int samples = mp3dec_decode_frame(decoder, file + frameStart, bytesLeft, outBuffer, &info);
        ESP_LOGI(TAG, "ch=%d, hz=%d, samples=%d", info.channels, info.hz, samples);
        frameStart += info.frame_bytes;
        if (samples == 0)
        {
            ESP_LOGW(TAG, "Samples==0!!");
            break;
        }
        if(info.channels==1){
           makeStereo(outBuffer, samples);
           samples*=2;
        }
        
        
        i2s_channel_write(tx_handle, outBuffer, sizeof(int16_t) * samples, nullptr, portMAX_DELAY);
    }
    i2s_channel_disable(tx_handle);
    ESP_LOGI(TAG, "Finished");
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}