#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include "esp32/rom/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include <Wire.h>
//#include "DFRobot_IIS.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include <bit>

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

#define ESP_SLAVE_ADDR              (0X34) //NAU8822 Slave Address
#include <stdint.h>


struct __attribute__((packed)) sWavHeader {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    uint32_t wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    uint32_t fmt_chunk_size; // Should be 16 for PCM
    uint16_t audio_format; // Should be 1 for PCM. 3 for IEEE Float
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    uint16_t sample_alignment; // num_channels * Bytes Per Sample
    uint16_t bit_depth; // Number of bits per sample
    
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

esp_err_t init_sdcard(void)
{
    char TAG[] = "sdcard";

    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
 
    slot_config.width = 4;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};

    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

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
    return ESP_OK;
}

void print_sdcard_info(void)
{
    char TAG[] = "sdcard";
    ESP_LOGI(TAG, "SD card info:");
    ESP_LOGI(TAG, "Name: %s", card->cid.name);
    ESP_LOGI(TAG, "Speed: %s", (card->csd.tr_speed < 25000000) ? "Default Speed" : "High Speed");
    ESP_LOGI(TAG, "Frequency: %ukHz", card->max_freq_khz);
    ESP_LOGI(TAG, "Log Bus Width: %u", card->log_bus_width);
    ESP_LOGI(TAG, "Read Block Length: %u", card->csd.read_block_len);
}

void i2sMcklkInit(unsigned int sampleRate)
{
    periph_module_enable(PERIPH_LEDC_MODULE);
    ledc_timer_bit_t bit_num = (ledc_timer_bit_t)2;
    int duty = pow(2, (int)bit_num) / 2;
    ledc_timer_config_t timer_conf = {};
    timer_conf.duty_resolution = bit_num;
    timer_conf.freq_hz = sampleRate * 256;
    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);
    ledc_channel_config_t ch_conf = {};
    ch_conf.channel = LEDC_CHANNEL_1;
    ch_conf.timer_sel = LEDC_TIMER_0;
    ch_conf.intr_type = LEDC_INTR_DISABLE;
    ch_conf.duty = duty;
    ch_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ch_conf.gpio_num = I2S_MCLK_PIN;
    ledc_channel_config(&ch_conf);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
}

void i2sMasterInit(uint32_t sampleRate, i2s_bits_per_sample_t bitsPerSample)
{
    i2s_config_t i2s_config = {};

    i2s_config.mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.sample_rate = sampleRate;
    i2s_config.bits_per_sample = bitsPerSample;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    i2s_config.dma_buf_count = 5;
    i2s_config.dma_buf_len = 100;
    i2s_config.use_apll = false;
    i2s_pin_config_t pin_config = {
        .bck_io_num = 5,
        .ws_io_num = 17,
        .data_out_num = 0,
        .data_in_num = 39};
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void i2cWriteNAU8822(int8_t addr, int16_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (NAU8822_I2C_ADDRESS) | I2C_MASTER_WRITE,  true);
    i2c_master_write_byte(cmd, ((addr << 1) | (data >> 8)) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, ((int8_t)(data & 0x00ff)) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 100);
    i2c_cmd_link_delete(cmd);
}

void i2cSetupNAU8822Play()
{
    i2cWriteNAU8822(0, 0x000);
    vTaskDelay(10);
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

void i2cInit(){
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

void setup()
{
    Serial.begin(115200);

    ESP_LOGI(TAG, "Starting up");

    ESP_ERROR_CHECK(init_sdcard());
    print_sdcard_info();

    i2cInit();
    


    

}

void loop()
{
    i2cSetupNAU8822Play();
    sWav_t *wav = new sWav_t();
    const char *filename = "/sdcard/CantinaBand3.wav";
    vTaskDelay(100);
    wav->fp = fopen(filename, "rb");
    if (wav->fp == NULL)
    {
        printf("playWAV(): Unable to open wav file. %s\n", filename);
        return;
    }
    if(fread(&(wav->header), 1, 44, wav->fp)!=44){
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

    //Hier weiter machen
    while (1)
    {
        if (fread(&wav->header.dataType1, 1, 1, wav->fp) != 1)
        {
            printf("playWAV(): Unable to read data chunk ID.\n");
            delete wav;
            break;
        }
        if (strncmp("d", wav->header.dataType1, 1) == 0)
        {
            fread(&wav->header.dataType2, 3, 1, wav->fp);
            if (strncmp("ata", wav->header.dataType2, 3) == 0)
            {
                fread(&(wav->header.dataSize), 4, 1, wav->fp);
                break;
            }
        }
    }
    i2sMcklkInit(wav->header.sampleRate);
    i2sMasterInit(wav->header.sampleRate, wav->header.bitsPerSample);
    i2s_set_sample_rates(I2S_NUM_0, wav->header.sampleRate); // doppelt! -->sollte schon im MasterInit passieren
    while (fread(&wav->header.test, 1, 800, wav->fp))
    {
        char *buf = (char *)&wav->header.test;
        int bytes_left = 800, bytes_written = 0;
        while (bytes_left > 0)
        {
            //  bytes_written = i2s_write(I2S_NUM_0 , buf ,((bits+8)/16)*SAMPLE_PER_CYCLE*4, bytes_left , 0);
            ESP_ERROR_CHECK(i2s_write(I2S_NUM_0, buf, ((bits + 8) / 16) * SAMPLE_PER_CYCLE * 4, &i2s_bytes_write, 100));
            bytes_left -= bytes_written;
            buf += bytes_written;
        }
        if (mark == STOP)
        {
 
            i2cWriteNAU8822(52, 0x140);
            i2cWriteNAU8822(53, 0x140);
            i2cWriteNAU8822(54, 0x140);
            i2cWriteNAU8822(55, 0x140);
       
            break;
        }
    }
    i2s_stop(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
    fclose(wav->fp);
    delete wav;
}
