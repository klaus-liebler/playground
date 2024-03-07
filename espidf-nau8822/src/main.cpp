#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <bit>
#include "esp32/rom/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h" // i2s setup
#include "driver/i2c.h"
#include "driver/ledc.h"

// #include "DFRobot_IIS.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <sys/stat.h>
#include <dirent.h>
#include <esp_log.h>
#define TAG "MAIN"


#define I2S_MCLK_PIN GPIO_NUM_22
#define I2S_BLK_PIN GPIO_NUM_5
#define I2S_WS_PIN GPIO_NUM_17
#define I2S_DATA_OUT_PIN GPIO_NUM_0
#define I2S_DATA_IN_PIN GPIO_NUM_NC
#define I2S_SCLK_PIN GPIO_NUM_NC

// SDMMC Configuration (default for ESP32-WROVER-E)
#define SDMMC_CMD_PIN GPIO_PIN_15
#define SDMMC_CLK_PIN GPIO_PIN_14
#define SDMMC_D0_PIN GPIO_PIN_2
#define SDMMC_D1_PIN GPIO_PIN_4
#define SDMMC_D2_PIN GPIO_PIN_12
#define SDMMC_D3_PIN GPIO_PIN_13

#define NAU8822_I2C_ADDRESS (0X34) // NAU8822 Slave Address
#define AUDIO_BUFFER 2048

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

uint8_t Volume1{50}; // Speaker
uint8_t Volume2{50}; // Headphone

sdmmc_card_t *card;
i2s_chan_handle_t tx_handle;

esp_err_t init_sdcard(void)
{

    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .disk_status_check_enable = false};
    const char* sdroot = "/sdcard";
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
    while ((entry = readdir(dir)) != nullptr) {
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
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),                                         // the wav file sample rate
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

esp_err_t  i2cReadNAU8822(uint8_t addr, uint16_t& reg_data)
{
    uint8_t write_buf[1] = {(uint8_t)((addr << 1))};
    uint8_t read_buf[2];
    esp_err_t espRc = i2c_master_write_read_device(I2C_NUM_0, NAU8822_I2C_ADDRESS, write_buf, sizeof(write_buf), read_buf, sizeof(read_buf), pdMS_TO_TICKS(1000));
    reg_data = read_buf[1]+(read_buf[0]<<8);
    return espRc;
}

esp_err_t i2cCheckNAU8822(){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (NAU8822_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    if(ret!=ESP_OK){
        ESP_LOGE(TAG, "NAU8822 does not respond");
        return ESP_FAIL;
    }
    uint16_t data{0};
    ESP_ERROR_CHECK(i2cReadNAU8822(0x3F, data));
    if(data!=0x1A){
        ESP_LOGE(TAG, "NAU8822 does respond the wrong id %d", data);
        return ESP_FAIL;
    }
    return ESP_OK;

}


void i2cSetupNAU8822Play()
{
    i2cWriteNAU8822(0, 0x000);//Software Reset
    vTaskDelay(pdMS_TO_TICKS(2));
    uint16_t value{0};
    i2cWriteNAU8822(1, 0x000);//Write something in any other register to release reset condition
    ESP_ERROR_CHECK(i2cReadNAU8822(1, value));
    if(value!=0){
        ESP_LOGE(TAG, "Power Management Resgister is not 0");
    }
    i2cWriteNAU8822(1, 0x15A);
    ESP_ERROR_CHECK(i2cReadNAU8822(1, value));
    if(value!=0x15A){
        ESP_LOGE(TAG, "Power Management Resgister is not 0x15A");
    }
    i2cWriteNAU8822(1, 0x1FF);//Enable everything
    ESP_ERROR_CHECK(i2cReadNAU8822(1, value));
    if(value!=0x1FF){
        ESP_LOGE(TAG, "Power Management Resgister is not 0x1FF");
    }
    i2cWriteNAU8822(2, 0x1BF); // Power 2: Enable everything, but sleep (so sleep is off)
    i2cWriteNAU8822(3, 0x1FF); // Power 3: Enable everything
    i2cWriteNAU8822(4, 0x010); //Audio Intrface: normal phase, 16bit (instead of default 24), standard I2S format
    i2cWriteNAU8822(5, 0x000); //no companding = reset value!
    i2cWriteNAU8822(6, 0x00C); //MCLK, pin#11 used as master clock, divide by 1,  Scaling of output frequency at BCLK pin#8 when chip is in master mode: divide by 8
    i2cWriteNAU8822(7, 0x000); //Clokc COntrol 2: default
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
    if (Volume2 != 0)
    {
        i2cWriteNAU8822(52, Volume2);
        i2cWriteNAU8822(53, Volume2 + 256);
    }
    else
    {
        i2cWriteNAU8822(52, 0x040);
        i2cWriteNAU8822(53, 0x040);
    }
    if (Volume1 != 0)
    {
        i2cWriteNAU8822(54, Volume1);
        i2cWriteNAU8822(55, Volume1 + 256);
    }
    else
    {
        i2cWriteNAU8822(54, 0x040);
        i2cWriteNAU8822(55, 0x040);
    }
}

void i2cInit()
{
    i2c_port_t i2c_master_port = I2C_NUM_0;
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = (gpio_num_t)26;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = (gpio_num_t)27;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    conf.clk_flags = 0;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void setSpeakersVolume(uint8_t volume)
{
    if (volume > 99)
    {
        Volume1 = 99;
    }
    if (volume < 1)
    {
        Volume1 = 0;
    }
    Volume1 = (volume * 64 / 100);
    i2cWriteNAU8822(54, Volume1);
    i2cWriteNAU8822(55, Volume1 + 256);
}

void muteSpeakers(void)
{
    i2cWriteNAU8822(54, 0x140);
    i2cWriteNAU8822(55, 0x140);
}

void setHeadphonesVolume(uint8_t volume)
{
    if (volume > 99)
    {
        Volume2 = 99;
    }
    if (volume < 1)
    {
        Volume2 = 0;
    }
    Volume2 = (volume * 64 / 100);
    i2cWriteNAU8822(52, Volume2);
    i2cWriteNAU8822(53, Volume2 + 256);
}

void muteHeadphones(void)
{
    i2cWriteNAU8822(52, 0x140);
    i2cWriteNAU8822(53, 0x140);
}

extern "C" void app_main()
{

    ESP_LOGI(TAG, "Starting up");
    if constexpr (std::endian::native == std::endian::big)
        ESP_LOGI(TAG, "big-endian");
    else if constexpr (std::endian::native == std::endian::little)
        ESP_LOGI(TAG, "little-endian");
    else
        ESP_LOGI(TAG, "mixed-endian");

    ESP_ERROR_CHECK(init_sdcard());
    print_sdcard_info();
    sdmmc_card_print_info(stdout, card);

    i2cInit();
    i2cSetupNAU8822Play();
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
        return; /* bad error "couldn't read RIFF_ID" */
    }
    if (strncmp("RIFF", wav->header.riff_header, 4))
    {
        printf("playWAV(): RIFF descriptor not found.\n");
        return;
    }
    if (strncmp("WAVE", wav->header.wave_header, 4))
    {
        printf("playWAV(): WAVE chunk ID not found.\n");
        return;
    }
    if (strncmp("fmt", wav->header.fmt_header, 3))
    {
        printf("playWAV(): fmt chunk format not found.\n");
        return;
    }

    wav->header.wav_size = std::byteswap(wav->header.wav_size);
    wav->header.fmt_chunk_size = std::byteswap(wav->header.fmt_chunk_size);
    wav->header.audio_format = std::byteswap(wav->header.audio_format);
    wav->header.num_channels = std::byteswap(wav->header.num_channels);
    wav->header.sample_rate = std::byteswap(wav->header.sample_rate);
    wav->header.byte_rate = std::byteswap(wav->header.byte_rate);
    wav->header.sample_alignment = std::byteswap(wav->header.sample_alignment);
    wav->header.bit_depth = std::byteswap(wav->header.bit_depth);
    wav->header.data_bytes = std::byteswap(wav->header.data_bytes);

    ESP_LOGI(TAG, "Overall size    : %lubyte", wav->header.wav_size);
    ESP_LOGI(TAG, "fmt_chunk_size  : %lubyte, (Should be 16 for PCM)", wav->header.fmt_chunk_size);
    ESP_LOGI(TAG, "audio_format    : %u, (Should be 1 for PCM. 3 for IEEE Float)", wav->header.audio_format);
    ESP_LOGI(TAG, "num_channels    : %u", wav->header.num_channels);
    ESP_LOGI(TAG, "sample_rate     : %luHz", wav->header.sample_rate);
    ESP_LOGI(TAG, "sample_alignment: %ubyte (num_channels * Bytes Per Sample)", wav->header.sample_alignment);
    ESP_LOGI(TAG, "data_bytes      : %lubyte (Number of samples * num_channels * sample byte size)", wav->header.data_bytes);
    ESP_LOGI(TAG, "bit_depth       : %ubit (Number of bits per sample)", wav->header.bit_depth);
    ESP_LOGI(TAG, "data_bytes      : %lubyte (Number of samples * num_channels * sample byte size)", wav->header.data_bytes);

    // i2sMcklkInit(wav->header.sample_rate);
    i2sMasterInit(wav->header.sample_rate, (i2s_data_bit_width_t)wav->header.bit_depth);


    int16_t *buf = new int16_t[AUDIO_BUFFER];
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, wav->fp);
    i2s_channel_enable(tx_handle);
    while (bytes_read > 0)
    {
        // write the buffer to the i2s
        i2s_channel_write(tx_handle, buf, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        bytes_read = fread(buf, sizeof(int16_t), AUDIO_BUFFER, wav->fp);
        ESP_LOGV(TAG, "Bytes read: %d", bytes_read);
    }
    i2s_channel_disable(tx_handle);
    delete[] buf;
    while (true)
    {
        ESP_LOGI(TAG, "Finished");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}