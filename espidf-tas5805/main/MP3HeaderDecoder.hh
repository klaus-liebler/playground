#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include "driver/i2s.h"
#include "mp3dec.h"
#include "mp3common.h"
#include <esp_log.h>

#define TAG "MP3"

namespace MP3
{
    constexpr size_t SAMPLES_PER_FRAME = 1152;
    constexpr size_t FRAME_MAX_SIZE_BYTES = 1024;
    constexpr size_t HEADER_LENGTH_BYTES = 4;
}

class MP3HeaderDecoder
{
private:
    HMP3Decoder decoder = nullptr;
    i2s_port_t i2s_num;
    int frameStart{0};
    unsigned char *frameBuffer;
    int16_t *outBuffer;
    const uint8_t *file{nullptr};
    size_t fileLen{0};

    int FindSyncWordOnAnyPosition(int offset)
    {
        for (int i = offset; i < fileLen - 1; i++)
        {
            if ((file[i + 0] & SYNCWORDH) == SYNCWORDH && (file[i + 1] & SYNCWORDL) == SYNCWORDL)
                return i;
        }
        return -1;
    }

public:
    esp_err_t Play(const uint8_t *file, size_t fileLen)
    {
        i2s_zero_dma_buffer(i2s_num);
        this->file = file;
        this->fileLen = fileLen;
        frameStart = FindSyncWordOnAnyPosition(0);
        if (frameStart == -1)
        {
            ESP_LOGE(TAG, "Did not find a single sync word in the whole file at %p with %zu bytes", file, fileLen);
            this->file = nullptr;
            this->fileLen = 0;
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "Successfully initialized a new sound play task. File=%p; FileLen=%zu; FrameStart=%d;", file, fileLen, frameStart);
        return ESP_OK;
    }

    esp_err_t Loop()
    {
        if (!file)
        {
            vTaskDelay(1);
            return ESP_OK;
        }

        if (frameStart == fileLen)
        {
            ESP_LOGI(TAG, "Reached End of File");
            file = nullptr;
            fileLen = 0;
            return ESP_OK;
        }
        unsigned char *inbuf = const_cast<unsigned char *>(file + frameStart);
        int bytesLeft = fileLen - frameStart;
        ESP_LOGD(TAG, "About to call MP3Decode %p %p %d %p", decoder, inbuf, bytesLeft, this->outBuffer);
        int decodingResult = MP3Decode(decoder, &inbuf, &bytesLeft, this->outBuffer, 0);
        this->frameStart = fileLen - bytesLeft;

        if (decodingResult != 0)
        {
            // decoding error
            ESP_LOGE(TAG, "MP3 decode error: %d: frameStart=%d; bytesLeft=%d; - skipping frame!", decodingResult, frameStart, bytesLeft);
            frameStart = FindSyncWordOnAnyPosition(frameStart + MP3::HEADER_LENGTH_BYTES);
            if (frameStart == -1)
                frameStart = fileLen;
            return ESP_FAIL;
        }
        MP3FrameInfo info;
        MP3GetLastFrameInfo(decoder, &info);
        ESP_LOGD(TAG, "frameStart=%d; bytesLeft=%d; samples=%d @ %d Hz", frameStart, bytesLeft, info.outputSamps, info.samprate); //417 or 418 bytes, 2304samples, 44100

        size_t i2s_bytes_write;
        i2s_write(i2s_num, this->outBuffer, info.outputSamps * 2, &i2s_bytes_write, portMAX_DELAY);
        if (i2s_bytes_write != info.outputSamps * 2)
        {
            ESP_LOGE(TAG, "i2s_bytes_write!=info.outputSamps*2");
        }
        return ESP_OK;
    }

    MP3HeaderDecoder(i2s_port_t i2s_num) : i2s_num(i2s_num)
    {
        this->frameBuffer = new uint8_t[MP3::FRAME_MAX_SIZE_BYTES];
        this->outBuffer = new int16_t[2 * MP3::SAMPLES_PER_FRAME];
    }

    esp_err_t Init(gpio_num_t bck, gpio_num_t ws, gpio_num_t data)
    {
        decoder = MP3InitDecoder();
        i2s_config_t i2s_config;
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX); // Only TX
        i2s_config.sample_rate = 44100;
        i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
        i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT; //2-channels
        i2s_config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
        i2s_config.intr_alloc_flags = 0; //Default interrupt priority
        i2s_config.dma_buf_count = 18;
        i2s_config.dma_buf_len = 128; //MP3::SAMPLES_PER_FRAME/2; //Max 1024
        i2s_config.use_apll = false;
        i2s_config.fixed_mclk = -1;
        i2s_config.tx_desc_auto_clear = true; //Auto clear tx descriptor on underflow

        ESP_ERROR_CHECK(i2s_driver_install(i2s_num, &i2s_config, 0, NULL));
        i2s_pin_config_t pin_config;
        pin_config.bck_io_num = (int)bck;
        pin_config.ws_io_num = (int)ws;
        pin_config.data_out_num = (int)data;
        pin_config.data_in_num = -1; //Not used
        ESP_ERROR_CHECK(i2s_set_pin(i2s_num, &pin_config));
        return ESP_OK;
    }
};