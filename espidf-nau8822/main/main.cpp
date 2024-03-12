#include <stdio.h>
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

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <sys/stat.h>
#include <dirent.h>
#include <esp_log.h>
#define TAG "MAIN"

enum class eNauInit
{
    ORIGINAL_DF_ROBOT,
    BTL,
    STEREO
};

constexpr eNauInit nauInit{eNauInit::BTL};

extern const uint8_t wav_start[] asm("_binary_CantinaBand3_wav_start");
extern const uint8_t wav_end[] asm("_binary_CantinaBand3_wav_end");

#if defined(CONFIG_IDF_TARGET_ESP32S3)
constexpr gpio_num_t I2C_SCL_PIN{GPIO_NUM_7};       // von IO27
constexpr gpio_num_t I2C_SDA_PIN{GPIO_NUM_6};       // von IO26
constexpr gpio_num_t I2S_MCLK_PIN{GPIO_NUM_16};     // von IO22
constexpr gpio_num_t I2S_BLK_PIN{GPIO_NUM_9};       // von IO5
constexpr gpio_num_t I2S_WS_PIN{GPIO_NUM_45};       // von IO17
constexpr gpio_num_t I2S_DATA_OUT_PIN{GPIO_NUM_10}; // von IO0
constexpr gpio_num_t I2S_DATA_IN_PIN{GPIO_NUM_NC};  //--
constexpr gpio_num_t SDMMC_CMD_PIN{GPIO_NUM_35};    // von IO15
constexpr gpio_num_t SDMMC_CLK_PIN{GPIO_NUM_36};    // von IO14
constexpr gpio_num_t SDMMC_D0_PIN{GPIO_NUM_37};     // von IO2
#elif defined(CONFIG_IDF_TARGET_ESP32)
constexpr gpio_num_t I2C_SCL_PIN{GPIO_NUM_27};      // von IO27
constexpr gpio_num_t I2C_SDA_PIN{GPIO_NUM_26};      // von IO26
constexpr gpio_num_t I2S_MCLK_PIN{GPIO_NUM_0};      // von IO22
constexpr gpio_num_t I2S_BLK_PIN{GPIO_NUM_5};       // von IO5
constexpr gpio_num_t I2S_WS_PIN{GPIO_NUM_17};       // von IO17
constexpr gpio_num_t I2S_DATA_OUT_PIN{GPIO_NUM_22}; // von IO0
constexpr gpio_num_t I2S_DATA_IN_PIN{GPIO_NUM_NC};  //--
constexpr gpio_num_t SDMMC_CMD_PIN{GPIO_NUM_15};    // von IO15
constexpr gpio_num_t SDMMC_CLK_PIN{GPIO_NUM_14};    // von IO14
constexpr gpio_num_t SDMMC_D0_PIN{GPIO_NUM_2};      // von IO2
#endif

constexpr uint8_t NAU8822_I2C_ADDRESS{0X1A};
constexpr size_t AUDIO_BUFFER_SIZE_IN_SAMPLES{2048};
constexpr size_t TARGET_CHANNELS{2};
#include <stdint.h>

struct __attribute__((packed)) sWavHeader
{
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint32_t wav_size;   // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"

    // Format Header
    char fmt_header[4];      // Contains "fmt " (includes trailing space)
    uint32_t fmt_chunk_size; // Should be 16 for PCM
    uint16_t audio_format;   // Should be 1 for PCM. 3 for IEEE Float
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;        // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint16_t sample_alignment; // num_channels * Bytes Per Sample
    uint16_t bit_depth;        // Number of bits per sample

    // Data
    char data_header[4]; // Contains "data"
    uint32_t data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
};

struct sWav_t
{
    sWavHeader header;
    FILE *fp;
};

sdmmc_card_t *card;
i2s_chan_handle_t tx_handle;

esp_err_t init_sdcard(void)
{

    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    slot_config.clk = SDMMC_CLK_PIN;
    slot_config.cmd = SDMMC_CMD_PIN;
    slot_config.d0 = SDMMC_D0_PIN;
#endif
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false};
    const char *sdroot = "/sdcard";
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(sdroot, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set format_if_mount_failed = true.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }
    struct dirent *entry;
    DIR *dir = opendir(sdroot);
    while ((entry = readdir(dir)) != nullptr)
    {
        ESP_LOGI(TAG, "%s %s", (entry->d_type == DT_DIR ? "DIR " : "FILE"), entry->d_name);
    }
    closedir(dir);

    return ESP_OK;
}

void print_sdcard_info(void)
{
    ESP_LOGI(TAG, "SD card info:");
    ESP_LOGI(TAG, "Name: %s", card->cid.name);
    ESP_LOGI(TAG, "Speed: %s", (card->csd.tr_speed < 25000000) ? "Default Speed" : "High Speed");
    ESP_LOGI(TAG, "Frequency: %ukHz", card->max_freq_khz);
    ESP_LOGI(TAG, "Log Bus Width: %u", card->log_bus_width);
    ESP_LOGI(TAG, "Read Block Length: %u", card->csd.read_block_len);
}

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

void setSpeakersVolume(uint8_t volume0_255)
{
    volume0_255 >>= 2;
    i2cWriteNAU8822(54, volume0_255);
    i2cWriteNAU8822(55, volume0_255 + 256);
}

void muteSpeakers(void)
{
    i2cWriteNAU8822(54, 0x040);
    i2cWriteNAU8822(55, 0x140);
}

void setHeadphonesVolume(uint8_t volume0_255)
{
    volume0_255 >>= 2;
    i2cWriteNAU8822(52, volume0_255);
    i2cWriteNAU8822(53, volume0_255 + 256);
}

void muteHeadphones(void)
{
    i2cWriteNAU8822(52, 0x040);
    i2cWriteNAU8822(53, 0x140);
}

void i2cSetupNAU8822Play(uint8_t initialVolume_0_255=57*4)//this is the hardware default volume
{
    if (nauInit == eNauInit::BTL)
    {
        ESP_ERROR_CHECK(i2cWriteNAU8822(0, 0x000)); // Software Reset
        vTaskDelay(pdMS_TO_TICKS(100));
        muteSpeakers(); // according to datasheet power up procedure
        muteHeadphones();
        i2cWriteNAU8822(1, 0b1'0000'1101); // according to datasheet power up procedure
        vTaskDelay(pdMS_TO_TICKS(250));    // according to datasheet power up procedure
        i2cWriteNAU8822(2, 0b1'1000'0000); // Power 2: Enable Headphones
        i2cWriteNAU8822(3, 0b0'0110'1111); // Power 3: Enable LR-speaker and LR-mixer and LR-DAC
        i2cWriteNAU8822(4, 0b0'0001'0000); // Audio Interface: normal phase, 16bit (instead of default 24), standard I2S format
        i2cWriteNAU8822(6, 0b0'0000'1100); // MCLK, pin#11 used as master clock, divided by 1, divide by 8, slave mode
        // i2cWriteNAU8822(10, 0b0'0000'1000); //Oversampling = x128 //Achtung, kein Softmute einstellen, sonst immer! stumm!
        // i2cWriteNAU8822(14, 0x108); //ADC Oversampling = x128
        i2cWriteNAU8822(43, 0b0'0001'0000); // Right Speaker Submixer -->BTL-Configuration
        i2cWriteNAU8822(49, 0b0'0101'1110); //Output Control: Left DAC output to RMIX!. Außerdem: Thermal shutdown enable, Speaker für 5V Versorgungsspannung optimiert.
        i2cWriteNAU8822(50, 0b0'0000'0001); // Left mixer.
        i2cWriteNAU8822(51, 0x000);         // Right mixer.
    }
    else if (nauInit == eNauInit::ORIGINAL_DF_ROBOT)
    {
        i2cWriteNAU8822(0, 0x000);
        vTaskDelay(pdMS_TO_TICKS(100));
        i2cWriteNAU8822(1, 0x1FF);
        i2cWriteNAU8822(2, 0x1BF);
        i2cWriteNAU8822(3, 0x1FF);
        i2cWriteNAU8822(4, 0x010);
        i2cWriteNAU8822(5, 0x000);
        i2cWriteNAU8822(6, 0x00C);
        i2cWriteNAU8822(7, 0x000);
        i2cWriteNAU8822(13, 0x09f);
        i2cWriteNAU8822(10, 0x008);
        i2cWriteNAU8822(14, 0x108);
        i2cWriteNAU8822(15, 0x0FF);
        i2cWriteNAU8822(16, 0x1FF);
        i2cWriteNAU8822(45, 0x0bf);
        i2cWriteNAU8822(46, 0x1bf);
        i2cWriteNAU8822(47, 0x175);
        i2cWriteNAU8822(49, 0x042);
        i2cWriteNAU8822(50, 0x1DD);
        i2cWriteNAU8822(51, 0x001);
    }

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

extern "C" void app_main()
{

    ESP_LOGI(TAG, "Starting up");
    i2cInit();
    i2cCheckNAU8822();
    i2cSetupNAU8822Play();

    /*
        ESP_ERROR_CHECK(init_sdcard());
        print_sdcard_info();
        sdmmc_card_print_info(stdout, card);
        sWav_t *wav = new sWav_t();
        const char *filename = "/sdcard/Cantina3.wav";
        vTaskDelay(100);
        wav->fp = fopen(filename, "rb");//oper read binary
        if (wav->fp == nullptr)
        {
            printf("playWAV(): Unable to open wav file. %s\n", filename);
            return;
        }
        if (fread(&(wav->header), 1, sizeof(sWavHeader), wav->fp) != sizeof(sWavHeader))
        {
            printf("playWAV(): couldn't read WAVE header\n");
            return; // bad error "couldn't read RIFF_ID"
        }
    */
    sWavHeader header;
    memcpy(&header, wav_start, sizeof(sWavHeader));

    if (strncmp("RIFF", header.riff_header, 4))
    {
        ESP_LOGW(TAG, "playWAV(): RIFF descriptor not found.\n");
        return;
    }
    if (strncmp("WAVE", header.wave_header, 4))
    {
        ESP_LOGW(TAG, "playWAV(): WAVE chunk ID not found.\n");
        return;
    }
    if (strncmp("fmt", header.fmt_header, 3))
    {
        ESP_LOGW(TAG, "playWAV(): fmt chunk format not found.\n");
        return;
    }
    ESP_LOGI(TAG, "Overall size    : %lubyte", header.wav_size);
    ESP_LOGI(TAG, "fmt_chunk_size  : %lubyte, (Should be 16 for PCM)", header.fmt_chunk_size);
    ESP_LOGI(TAG, "audio_format    : %u, (Should be 1 for PCM. 3 for IEEE Float)", header.audio_format);
    ESP_LOGI(TAG, "num_channels    : %u", header.num_channels);
    ESP_LOGI(TAG, "sample_rate     : %luHz", header.sample_rate);
    ESP_LOGI(TAG, "sample_alignment: %ubyte (num_channels * Bytes Per Sample)", header.sample_alignment);
    ESP_LOGI(TAG, "data_bytes      : %lubyte (Number of samples * num_channels * sample byte size)", header.data_bytes);
    ESP_LOGI(TAG, "bit_depth       : %ubit (Number of bits per sample)", header.bit_depth);
    ESP_LOGI(TAG, "data_bytes      : %lubyte (Number of samples * num_channels * sample byte size)", header.data_bytes);

    i2sMasterInit(header.sample_rate, (i2s_data_bit_width_t)header.bit_depth);
    if (header.num_channels < 1 || header.num_channels > 2)
    {
        ESP_LOGE(TAG, "Only 1- or 2-channel input is supported. This file has  %u channels ", header.num_channels);
        return;
    }
    if (header.bit_depth != 16)
    {
        ESP_LOGE(TAG, "Only 16bit input is supported. This file has  %ubit ", header.bit_depth);
        return;
    }

    

    i2s_channel_enable(tx_handle);

    int16_t *read_buf = new int16_t[TARGET_CHANNELS * AUDIO_BUFFER_SIZE_IN_SAMPLES];
    size_t bytes_written = 0;
    const uint8_t *pos = wav_start + sizeof(sWavHeader);

    

    while (pos < wav_end)
    {
        size_t samplesAvailable = std::min(AUDIO_BUFFER_SIZE_IN_SAMPLES, (unsigned int)(wav_end - pos) / 2);

        memcpy(read_buf, pos, header.sample_alignment * samplesAvailable);
        int16_t min{INT16_MAX};
        int16_t max{INT16_MIN};
        
        for(int i=0;i<samplesAvailable;i++){
            //read_buf[i]*=3;
            min=std::min(min, read_buf[i]);
            max=std::max(max, read_buf[i]);
        }

        size_t samplesToWrite = samplesAvailable;
        if (header.num_channels == 1)
        {
            makeStereo(read_buf, samplesAvailable);
            samplesToWrite *= 2;
        }
        
        i2s_channel_write(tx_handle, read_buf, sizeof(int16_t) * samplesToWrite, &bytes_written, portMAX_DELAY);
        ESP_LOGI(TAG, "samplesAvailable %u, bytesWritten %u, min %i, max %i", samplesAvailable, bytes_written, min, max);

        pos += (samplesAvailable * 2);
    }
    i2s_channel_disable(tx_handle);
    delete[] read_buf;
    ESP_LOGI(TAG, "Finished");
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}