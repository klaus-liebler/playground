#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <esp_log.h>
#include "errorcodes.hh"
#define TAG "LCD"

/*
Idee: Den Auftrag, einen bestimmten Bereich des Displays neu zu befüllen, wird asynchron über einen AsyncRenderer erledigt.
Dieser wird in der sechsten Phase der Transaktion aktiv und übernimmt dann
Achtung. Wenn der Renderer mal false zurück liefert, müssen die Datenstrukturen auf dem Heap wieder entfernt werden, also insbesondere der Renderer und die Buffer

*/

namespace SPILCD16
{
    constexpr bool POST_CALLBACK_ACTIVE{true};
    constexpr size_t PIXEL_BUFFER_SIZE_IN_PIXELS = 240 * 8; // Wenn die volle Breite ausgenutzt würde, könnten 16 Zeilen "auf einen Rutsch" übertragen werden
    constexpr int SPI_Command_Mode = 0;
    constexpr int SPI_Data_Mode = 1;
    namespace RGB565
    {
        constexpr uint16_t BLACK = 0x0000;  /*   0,   0,   0 */
        constexpr uint16_t BLUE = 0x001F;   /*   0,   0, 255 */
        constexpr uint16_t GREEN = 0x07E0;  /*   0, 255,   0 */
        constexpr uint16_t RED = 0xF800;    /* 255,   0,   0 */
        constexpr uint16_t YELLOW = 0xFFE0; /* 255, 255,   0 */
        constexpr uint16_t WHITE = 0xFFFF;  /* 255, 255, 255 */
        constexpr uint16_t SILVER = 0xC618; /* 192, 192, 192 */
    }
    namespace CMD
    {
        // ST7789 specific commands used in init
        constexpr uint8_t NOP = 0x00;
        constexpr uint8_t SWRESET = 0x01;
        constexpr uint8_t RDDID = 0x04;
        constexpr uint8_t RDDST = 0x09;

        constexpr uint8_t RDDPM = 0x0A;      // Read display power mode
        constexpr uint8_t RDD_MADCTL = 0x0B; // Read display MADCTL
        constexpr uint8_t RDD_COLMOD = 0x0C; // Read display pixel format
        constexpr uint8_t RDDIM = 0x0D;      // Read display image mode
        constexpr uint8_t RDDSM = 0x0E;      // Read display signal mode
        constexpr uint8_t RDDSR = 0x0F;      // Read display self-diagnostic result (ST7789V)

        constexpr uint8_t SLPIN = 0x10;
        constexpr uint8_t SLPOUT = 0x11;
        constexpr uint8_t PTLON = 0x12;
        constexpr uint8_t NORON = 0x13;

        constexpr uint8_t INVOFF = 0x20;
        constexpr uint8_t INVON = 0x21;
        constexpr uint8_t GAMSET = 0x26; // Gamma set
        constexpr uint8_t DISPOFF = 0x28;
        constexpr uint8_t DISPON = 0x29;
        constexpr uint8_t CASET = 0x2A;
        constexpr uint8_t RASET = 0x2B;
        constexpr uint8_t RAMWR = 0x2C;
        constexpr uint8_t RGBSET = 0x2D; // Color setting for 4096, 64K and 262K colors
        constexpr uint8_t RAMRD = 0x2E;

        constexpr uint8_t PTLAR = 0x30;
        constexpr uint8_t VSCRDEF = 0x33; // Vertical scrolling definition (ST7789V)
        constexpr uint8_t TEOFF = 0x34;   // Tearing effect line off
        constexpr uint8_t TEON = 0x35;    // Tearing effect line on
        constexpr uint8_t MADCTL = 0x36;  // Memory data access control
        constexpr uint8_t IDMOFF = 0x38;  // Idle mode off
        constexpr uint8_t IDMON = 0x39;   // Idle mode on
        constexpr uint8_t RAMWRC = 0x3C;  // Memory write continue (ST7789V)
        constexpr uint8_t RAMRDC = 0x3E;  // Memory read continue (ST7789V)
        constexpr uint8_t COLMOD = 0x3A;

        constexpr uint8_t RAMCTRL = 0xB0;   // RAM control
        constexpr uint8_t RGBCTRL = 0xB1;   // RGB control
        constexpr uint8_t PORCTRL = 0xB2;   // Porch control
        constexpr uint8_t FRCTRL1 = 0xB3;   // Frame rate control
        constexpr uint8_t PARCTRL = 0xB5;   // Partial mode control
        constexpr uint8_t GCTRL = 0xB7;     // Gate control
        constexpr uint8_t GTADJ = 0xB8;     // Gate on timing adjustment
        constexpr uint8_t DGMEN = 0xBA;     // Digital gamma enable
        constexpr uint8_t VCOMS = 0xBB;     // VCOMS setting
        constexpr uint8_t LCMCTRL = 0xC0;   // LCM control
        constexpr uint8_t IDSET = 0xC1;     // ID setting
        constexpr uint8_t VDVVRHEN = 0xC2;  // VDV and VRH command enable
        constexpr uint8_t VRHS = 0xC3;      // VRH set
        constexpr uint8_t VDVSET = 0xC4;    // VDV setting
        constexpr uint8_t VCMOFSET = 0xC5;  // VCOMS offset set
        constexpr uint8_t FRCTR2 = 0xC6;    // FR Control 2
        constexpr uint8_t CABCCTRL = 0xC7;  // CABC control
        constexpr uint8_t REGSEL1 = 0xC8;   // Register value section 1
        constexpr uint8_t REGSEL2 = 0xCA;   // Register value section 2
        constexpr uint8_t PWMFRSEL = 0xCC;  // PWM frequency selection
        constexpr uint8_t PWCTRL1 = 0xD0;   // Power control 1
        constexpr uint8_t VAPVANEN = 0xD2;  // Enable VAP/VAN signal output
        constexpr uint8_t CMD2EN = 0xDF;    // Command 2 enable
        constexpr uint8_t PVGAMCTRL = 0xE0; // Positive voltage gamma control
        constexpr uint8_t NVGAMCTRL = 0xE1; // Negative voltage gamma control
        constexpr uint8_t DGMLUTR = 0xE2;   // Digital gamma look-up table for red
        constexpr uint8_t DGMLUTB = 0xE3;   // Digital gamma look-up table for blue
        constexpr uint8_t GATECTRL = 0xE4;  // Gate control
        constexpr uint8_t SPI2EN = 0xE7;    // SPI2 enable
        constexpr uint8_t PWCTRL2 = 0xE8;   // Power control 2
        constexpr uint8_t EQCTRL = 0xE9;    // Equalize time control
        constexpr uint8_t PROMCTRL = 0xEC;  // Program control
        constexpr uint8_t PROMEN = 0xFA;    // Program mode enable
        constexpr uint8_t NVMSET = 0xFC;    // NVM setting
        constexpr uint8_t PROMACT = 0xFE;   // Program action
    };

    typedef uint16_t Color;

    enum class TransactionPhase
    {
        DAT_WRITE_B0 = 0, //0 und 1 sind Buffer-Operationen. Ansonsten: gerade Zahlen sind Commands, ungerade Zahlen sind Data
        DAT_WRITE_B1 = 1,
        CMD_CA_SET = 2,
        DAT_CA_SET = 3,
        CMD_RA_SET = 4,
        DAT_RA_SET = 5,
        CMD_WRITE  = 6,
        DAT_UNSPECIFIED=7,
        CMD_UNSPECIFIED=8,

    };

    class Point2D
    {
    public:
        int16_t x;
        int16_t y;
        void CopyFrom(Point2D &other)
        {
            this->x = other.x;
            this->y = other.y;
        }
        uint32_t PixelsTo(Point2D other){
            return (std::max(0, other.x-x))*(std::max(0, other.y-y));
        }
        Point2D(int16_t x, int16_t y) : x(x), y(y) {}
        Point2D() : x(0), y(0) {}
    };
    /*
        Manager befragt AsyncRenderer zunächst nach dem rechteckigen Bereich, der beschrieben werden soll. Möglicherweise sind das mehrere Bereiche. Solange GetNextOverallLimits "true" zurück gibt, gibt es auch weitere zu beschreibende Bereiche
        Dann wird die Anzahl der Pixel für diesen Bereich ausgerechnet und Render eben so oft aufgerufen, bis diesen Bereich mit den zur Verfügung stehenden Buffern beschrieben werden kann
    */
    class AsyncRenderer
    {
    public:
        virtual bool GetNextOverallLimits(Point2D &start, Point2D &end_excl) = 0;
        virtual void Render(uint32_t startPixel, uint16_t *buffer, size_t len) = 0;
    };

    class FilledRectRenderer : public AsyncRenderer
    {
    private:
        Point2D start;
        Point2D end_excl;
        Color foreground;
        bool getOverallLimitsCalled{false};

    public:
        FilledRectRenderer(Point2D start, Point2D end_excl, Color foreground) : start(start), end_excl(end_excl), foreground(foreground) {}

        bool GetNextOverallLimits(Point2D &start, Point2D &end_excl) override
        {
            if(getOverallLimitsCalled) return false;
            start.CopyFrom(this->start);
            end_excl.CopyFrom(this->end_excl);
            getOverallLimitsCalled=true;
            return true;
        }

        void Render(uint32_t startPixel, uint16_t *buffer, size_t len) override
        {
            memset(buffer, 0, len);
            for(int i=0; i<len; i++){
                buffer[i] = this->foreground;
            }
        }
    };

    class TransactionInfo;

    class TransactionCallbackInterface{
        public:
            virtual void preCb(spi_transaction_t *t, TransactionInfo*)=0;
            virtual void postCb(spi_transaction_t *t, TransactionInfo*)=0;
    };

/**
 * TransactionInfo muss für jede mögliche Transaktion mit den erforderlichen Daten bestückt sein
*/
    class TransactionInfo
    {
    public:
        TransactionPhase phase;
        TransactionCallbackInterface *manager;
        AsyncRenderer *renderer;
        uint32_t currentPixel;
        uint32_t pixelsInCurrentRect;
        TransactionInfo(TransactionPhase phase, TransactionCallbackInterface *manager, AsyncRenderer *renderer, uint32_t currentPixel, uint32_t pixelsInCurrentRect) : phase(phase), manager(manager), renderer(renderer), currentPixel(currentPixel), pixelsInCurrentRect(pixelsInCurrentRect) {}
    };

    template <spi_host_device_t spiHost, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, uint16_t WIDTH, uint16_t HEIGHT, uint16_t OFFSET_X, uint16_t OFFSET_Y>
    class M : public TransactionCallbackInterface
    {
    private:
        spi_device_handle_t spi;
        uint16_t backgroundColor;
        uint16_t foregroundColor;
        uint16_t *buffer[2];
        spi_transaction_t PREPARE_TRANSACTIONS[5];
        spi_transaction_t BUFFER_TRANSACTIONS[2];
        SemaphoreHandle_t xSemaphore;
        

        // For Debugging purposes
        int preCbCalls = 0;
        int postCb1Calls = 0;
        int postCb2Calls = 0;

        void delayMS(int ms)
        {
            vTaskDelay((ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS);
        }

        void lcd_bytes_b(const uint8_t *data, size_t len)
        {
            spi_transaction_t trans = {};
            memset(&trans, 0, sizeof(spi_transaction_t));
            trans.length = len * 8;
            trans.tx_buffer = data;
            assert(spi_device_transmit(spi, &trans)==ESP_OK);
        }

        void lcd_cmd_b(const uint8_t cmd)
        {
            gpio_set_level(dc, SPI_Command_Mode);
            lcd_bytes_b(&cmd, 1);
        }

        void lcd_data_b(const uint8_t *data, size_t len)
        {
            gpio_set_level(dc, SPI_Data_Mode);
            lcd_bytes_b(data, len);
        }

        void lcd_data_b(const uint8_t data)
        {
            gpio_set_level(dc, SPI_Data_Mode);
            lcd_bytes_b(&data, 1);
        }

        void lcd_addr_b(uint16_t addr1, uint16_t addr2)
        {
            uint8_t Byte[4];
            Byte[0] = (addr1 >> 8) & 0xFF;
            Byte[1] = addr1 & 0xFF;
            Byte[2] = (addr2 >> 8) & 0xFF;
            Byte[3] = addr2 & 0xFF;
            lcd_data_b(Byte, 4);
        }

        void write_color_b(uint16_t color, size_t size)
        {
            assert(buffer[0]);
            int index = 0;
            uint8_t* b8 = (uint8_t*)buffer[0];
            for (int i = 0; i < size; i++)
            {
                b8[index++] = (color >> 8) & 0xFF;
                b8[index++] = color & 0xFF;
            }
            lcd_data_b(b8, size * 2);
        }

        void write_colors_b(uint16_t *colors, size_t size)
        {
            assert(buffer[0]);
            int index = 0;
            uint8_t* b8 = (uint8_t*)buffer[0];
            for (int i = 0; i < size; i++)
            {
                b8[index++] = (colors[i] >> 8) & 0xFF;
                b8[index++] = colors[i] & 0xFF;
            }
            lcd_data_b(b8, size * 2);
        }

        void lcd_cmd(const uint8_t cmd)
        {
            esp_err_t ret;
            spi_transaction_t t = {};
            memset(&t, 0, sizeof(spi_transaction_t));
            t.length = 8;       // Command is 8 bits
            t.tx_buffer = &cmd; // The data is the cmd itself
            t.user = (void *)new TransactionInfo(TransactionPhase::CMD_UNSPECIFIED, this, nullptr, 0, 0, nullptr, 0);
            ret = spi_device_transmit(spi, &t); // Transmit!
            assert(ret == ESP_OK);              // Should have had no issues.
        }

        void lcd_data(const uint8_t *data, int len)
        {
            esp_err_t ret;
            spi_transaction_t t = {};
            memset(&t, 0, sizeof(spi_transaction_t));
            if (len == 0)
                return;         // no need to send anything
            t.length = len * 8; // Len is in bytes, transaction length is in bits.
            t.tx_buffer = data; // Data
            t.user = (void *)new TransactionInfo(TransactionPhase::DAT_UNSPECIFIED, this, nullptr, 0, 0, nullptr);
            ret = spi_device_transmit(spi, &t); // Transmit!
            assert(ret == ESP_OK);              // Should have had no issues.
        }

        void updateAndEnqueuePrepareTransactions(Point2D start, Point2D end_ex){
            PREPARE_TRANSACTIONS[1].tx_data[0] = (start.x) >> 8;    // Start Col High
            PREPARE_TRANSACTIONS[1].tx_data[1] = (start.x) & 0xff;  // Start Col Low
            PREPARE_TRANSACTIONS[1].tx_data[2] = (end_ex.x) >> 8;   // End Col High
            PREPARE_TRANSACTIONS[1].tx_data[3] = (end_ex.x) & 0xff; // End Col Low
            PREPARE_TRANSACTIONS[3].tx_data[0] = (start.y) >> 8;    // Start Col High
            PREPARE_TRANSACTIONS[3].tx_data[1] = (start.y) & 0xff;  // Start Col Low
            PREPARE_TRANSACTIONS[3].tx_data[2] = (end_ex.y) >> 8;   // End Col High
            PREPARE_TRANSACTIONS[3].tx_data[3] = (end_ex.y) & 0xff; // End Col Low
            for (int ti = 0; ti < 5; ti++)
            {
                ESP_ERROR_CHECK(spi_device_queue_trans(spi, &PREPARE_TRANSACTIONS[ti], portMAX_DELAY));
            }

        }

        void updateAndEnqueueBufferTransaction(int index, AsyncRenderer *renderer, uint32_t startPixel, size_t pixelsRendered, size_t pixelsInCurrentRect){
            assert(index<2);
            BUFFER_TRANSACTIONS[index].length = 16 * pixelsRendered; // Data length, in bits
            BUFFER_TRANSACTIONS[index].rxlength=0;
            BUFFER_TRANSACTIONS[index].user = (void *)new TransactionInfo((TransactionPhase)(index), this, renderer, startPixel, pixelsInCurrentRect);
            ESP_ERROR_CHECK(spi_device_queue_trans(spi, &BUFFER_TRANSACTIONS[index], portMAX_DELAY));
        }

      
    public:
        M()
        {
            xSemaphore = xSemaphoreCreateBinary();
            xSemaphoreGive(xSemaphore);

            for (int i = 0; i < 2; i++)
            {
                buffer[i] = (uint16_t *)heap_caps_malloc(PIXEL_BUFFER_SIZE_IN_PIXELS * 2, MALLOC_CAP_DMA);
                assert(buffer[i] != NULL);
            }
            memset(PREPARE_TRANSACTIONS, 0, 5*sizeof(spi_transaction_t));
            
            PREPARE_TRANSACTIONS[0].length = 8;
            PREPARE_TRANSACTIONS[0].user = (void *)new TransactionInfo(TransactionPhase::CMD_CA_SET, this, nullptr, 0, 0);
            PREPARE_TRANSACTIONS[0].tx_data[0] = 0x2A; // Column Address Set
            PREPARE_TRANSACTIONS[0].flags = SPI_TRANS_USE_TXDATA;
            
            PREPARE_TRANSACTIONS[1].length = 8 * 4;
            PREPARE_TRANSACTIONS[1].user = (void *)new TransactionInfo(TransactionPhase::DAT_CA_SET, this, nullptr, 0, 0);
            PREPARE_TRANSACTIONS[1].flags = SPI_TRANS_USE_TXDATA;
            
            PREPARE_TRANSACTIONS[2].length = 8;
            PREPARE_TRANSACTIONS[2].user = (void *)new TransactionInfo(TransactionPhase::CMD_RA_SET, this, nullptr, 0, 0);
            PREPARE_TRANSACTIONS[2].tx_data[0] = 0x2B; // Row Address Set
            PREPARE_TRANSACTIONS[2].flags = SPI_TRANS_USE_TXDATA;

            PREPARE_TRANSACTIONS[3].length = 8 * 4;
            PREPARE_TRANSACTIONS[3].user = (void *)new TransactionInfo(TransactionPhase::DAT_RA_SET, this, nullptr, 0, 0);
            PREPARE_TRANSACTIONS[3].flags = SPI_TRANS_USE_TXDATA;
            
            PREPARE_TRANSACTIONS[4].length = 8;
            PREPARE_TRANSACTIONS[4].user = (void *)new TransactionInfo(TransactionPhase::CMD_WRITE, this, nullptr, 0, 0);
            PREPARE_TRANSACTIONS[4].tx_data[0] = 0x2C; // memory write
            PREPARE_TRANSACTIONS[4].flags = SPI_TRANS_USE_TXDATA;
            
            memset(BUFFER_TRANSACTIONS, 0, 2*sizeof(spi_transaction_t));
            BUFFER_TRANSACTIONS[0].tx_buffer = buffer[0];           
            BUFFER_TRANSACTIONS[1].tx_buffer = buffer[1];           
            
        }

        void preCb(spi_transaction_t *t, TransactionInfo *ti)
        {
            //größer 1 und gerade ist CMD
            if ((int)(ti->phase) > 1 && (int)(ti->phase)%2==0)
            {
                gpio_set_level(dc, SPI_Command_Mode);
            }
            else
            {
                gpio_set_level(dc, SPI_Data_Mode);
            }
            preCbCalls++;
        }

        void postCb(spi_transaction_t *t, TransactionInfo *ti)
        {

            if(!POST_CALLBACK_ACTIVE) return;
            TransactionPhase nextPhase;
            uint16_t *buf;
            if (ti->phase == TransactionPhase::DAT_WRITE_B0)
            {
                nextPhase = TransactionPhase::DAT_WRITE_B1;
                buf = this->buffer[0];
                postCb1Calls++;
            }
            else if (ti->phase == TransactionPhase::DAT_WRITE_B1)
            {
                nextPhase = TransactionPhase::DAT_WRITE_B0;
                buf = this->buffer[0]; // TODO buf[1]
                postCb1Calls++;
            }
            else
            {
                postCb2Calls++;
                return;
            }
            uint16_t nextPixel = ti->currentPixel+(t->length/16);

            SPILCD16::Point2D start;
            SPILCD16::Point2D end_excl;
            ti->renderer->GetNextOverallLimits(start, end_excl);
            if (ti->currentY + ti->yPerCallback >= end_excl.y)
            {
                delete[](ti->transactions);
                delete (ti);
            }
            ti->phase = nextPhase;
            ti->currentY += ti->yPerCallback;
            size_t pixelsRendered = ti->renderer->Render(ti->currentY, ti->yPerCallback, buf);

            // ti->transactions[5].tx_buffer = buf;         //finally send the line data
            ti->transactions[5].length = 8 * 2 * pixelsRendered; // Data length, in bits
            ESP_ERROR_CHECK(spi_device_queue_trans(spi, &ti->transactions[5], portMAX_DELAY));
            xSemaphoreGive(xSemaphore);
        }


        void Loop()
        {
            ESP_LOGI(TAG, "preCb %d, postCb1 %d, postCb2 %d", this->preCbCalls, postCb1Calls, postCb2Calls);
        }

        void Init_ST7789(uint16_t fillColor=RGB565::BLACK)
        {
            if (cs != GPIO_NUM_NC)
            {
                gpio_reset_pin(cs);
                gpio_set_direction(cs, GPIO_MODE_OUTPUT);
                gpio_set_level(cs, 0);
            }

            if (bl != GPIO_NUM_NC)
            {
                gpio_set_direction(bl, GPIO_MODE_OUTPUT);
            }
            gpio_reset_pin(dc);
            gpio_set_direction(dc, GPIO_MODE_OUTPUT);
            gpio_set_level(dc, 0);

            // Reset the display
            if (rst != GPIO_NUM_NC)
            {
                gpio_reset_pin(rst);
                gpio_set_direction(rst, GPIO_MODE_OUTPUT);
                gpio_set_level(rst, 0);
                delayMS(100);
                gpio_set_level(rst, 1);
                delayMS(100);
            }

            if (bl != GPIO_NUM_NC)
            {
                gpio_reset_pin(bl);
                gpio_set_direction(bl, GPIO_MODE_OUTPUT);
                gpio_set_level(bl, 1);
            }

            spi_bus_config_t buscfg = {};
            memset(&buscfg, 0, sizeof(buscfg));
            buscfg.miso_io_num = GPIO_NUM_NC;
            buscfg.mosi_io_num = mosi;
            buscfg.sclk_io_num = sclk;
            buscfg.quadwp_io_num = GPIO_NUM_NC;
            buscfg.quadhd_io_num = GPIO_NUM_NC;
            buscfg.max_transfer_sz = 0;
            buscfg.flags = 0;
            buscfg.intr_flags = 0;
            ESP_ERROR_CHECK(spi_bus_initialize(spiHost, &buscfg, SPI_DMA_CH_AUTO));

            spi_device_interface_config_t devcfg = {};
            memset(&devcfg, 0, sizeof(devcfg));
            devcfg.clock_speed_hz = SPI_MASTER_FREQ_10M; // 10 * 1000 * 1000; //Clock out at 10 MHz
            devcfg.mode = cs == GPIO_NUM_NC ? 3 : 0;
            devcfg.spics_io_num = cs;
            devcfg.queue_size = 7;
            devcfg.pre_cb = M::lcd_spi_pre_transfer_callback;
            devcfg.post_cb = M::lcd_spi_post_transfer_callback;

            ESP_ERROR_CHECK(spi_bus_add_device(spiHost, &devcfg, &spi));
            assert(spi);

            lcd_cmd_b(0x01); // Software Reset
            delayMS(150);

            lcd_cmd_b(0x11); // Sleep Out
            delayMS(255);

            lcd_cmd_b(0x3A); // Interface Pixel Format
            lcd_data_b(0x55);
            delayMS(10);

            lcd_cmd_b(0x36); // Memory Data Access Control
            lcd_data_b(0x00);

            lcd_cmd_b(0x2A); // Column Address Set
            lcd_data_b(0x00);
            lcd_data_b(0x00);
            lcd_data_b(0x00);
            lcd_data_b(0xF0);

            lcd_cmd_b(0x2B); // Row Address Set
            lcd_data_b(0x00);
            lcd_data_b(0x00);
            lcd_data_b(0x00);
            lcd_data_b(0xF0);

            lcd_cmd_b(0x21); // Display Inversion On
            delayMS(10);

            lcd_cmd_b(0x13); // Normal Display Mode On
            delayMS(10);

            lcd_cmd_b(0x29); // Display ON
            delayMS(255);

            FilledRectRenderer rr(Point2D(0,0), Point2D(WIDTH, HEIGHT), backgroundColor);
            DrawAsyncAsSync(&rr);
        }

        ErrorCode FillRect(Point2D start, Point2D end_ex, Color col)
        {

            if (start.x >= end_ex.x || start.y >= end_ex.y)
                return ErrorCode::GENERIC_ERROR;
            size_t pixels = (end_ex.x - start.x) * (end_ex.y - start.y);
            ESP_LOGI(TAG, "Called FillRect for %d/%d - %d/%d, aka %d pixels of %d", start.x, start.y, end_ex.x, end_ex.y, pixels, PIXEL_BUFFER_SIZE_IN_PIXELS);
            if (pixels > PIXEL_BUFFER_SIZE_IN_PIXELS)
            {
                return ErrorCode::GENERIC_ERROR;
            }
            spi_transaction_t trans = {};
            memset(&trans, 0, sizeof(spi_transaction_t));
            trans.length = 8;
            trans.user = (void *)new TransactionInfo(TransactionPhase::CMD_UNSPECIFIED, this, nullptr, 0, 0);
            trans.tx_data[0] = 0x2A; // Column Address Set
            trans.flags = SPI_TRANS_USE_TXDATA;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8 * 4;
            trans.user = (void *)new TransactionInfo(TransactionPhase::DAT_UNSPECIFIED, this, nullptr, 0 , 0);
            trans.tx_data[0] = (start.x) >> 8;    // Start Col High
            trans.tx_data[1] = (start.x) & 0xff;  // Start Col Low
            trans.tx_data[2] = (end_ex.x) >> 8;   // End Col High
            trans.tx_data[3] = (end_ex.x) & 0xff; // End Col Low
            trans.flags = SPI_TRANS_USE_TXDATA;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8;
            trans.user = (void *)new TransactionInfo(TransactionPhase::CMD_UNSPECIFIED, this, nullptr, 0, 0);
            trans.tx_data[0] = 0x2B; // Row Address Set
            trans.flags = SPI_TRANS_USE_TXDATA;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8 * 4;
            trans.user = (void *)new TransactionInfo(TransactionPhase::DAT_UNSPECIFIED, this, nullptr, 0, 0);
            trans.tx_data[0] = (start.y) >> 8;    // Start page High
            trans.tx_data[1] = (start.y) & 0xff;  // Start page Low
            trans.tx_data[2] = (end_ex.y) >> 8;   // End page High
            trans.tx_data[3] = (end_ex.y) & 0xff; // End page Low
            trans.flags = SPI_TRANS_USE_TXDATA;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8;
            trans.user = (void *)new TransactionInfo(TransactionPhase::CMD_UNSPECIFIED, this, nullptr, 0, 0);
            trans.tx_data[0] = 0x2C; // memory write
            trans.flags = SPI_TRANS_USE_TXDATA;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            for (int i = 0; i < pixels; i++)
            {
                buffer[0][i] = col;
            }

            trans.length = 8 * 2 * pixels; // Data length, in bits
            trans.user = (void *)new TransactionInfo(TransactionPhase::DAT_UNSPECIFIED, this, nullptr, 0, 0);
            trans.tx_buffer = buffer[0]; // finally send the line data

            trans.flags = 0;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));
            ESP_LOGI(TAG, "FillRect finished");
            return ErrorCode::OK;
        }

        ErrorCode DrawAsync(AsyncRenderer *renderer)
        {
            
            xSemaphoreTake(xSemaphore);
            Point2D start;
            Point2D end_ex;
            if(!renderer->GetNextOverallLimits(start, end_ex)){
                return ErrorCode::GENERIC_ERROR;
            }
            if (start.x >= end_ex.x || start.y >= end_ex.y)
                return ErrorCode::GENERIC_ERROR;
            uint32_t pixelsMax = start.PixelsTo(end_ex);
            ESP_LOGI(TAG, "Called DrawAsyncAsSync for %d/%d - %d/%d with %d Pixels", start.x, start.y, end_ex.x, end_ex.y, pixelsMax);
            updateAndEnqueuePrepareTransactions(start, end_ex);
            ESP_LOGI(TAG, "Five basic transaction have been enqueued.");
            uint32_t pixel{0};
           
            uint32_t pixelsToWriteNow = std::min(pixelsMax-pixel, PIXEL_BUFFER_SIZE_IN_PIXELS);
            if(pixelsToWriteNow>0){
                renderer->Render(pixel, buffer[0], pixelsToWriteNow);
                ESP_LOGI(TAG, "Buffer0 transaction will be enqueued with %d pixels. Waiting for completion", pixelsToWriteNow);
                updateAndEnqueueBufferTransaction(0, renderer, pixel, pixelsToWriteNow);
                pixel+=pixelsToWriteNow;
            }
            pixelsToWriteNow = std::min(pixelsMax-pixel, PIXEL_BUFFER_SIZE_IN_PIXELS);
            if(pixelsToWriteNow>0){
                renderer->Render(pixel, buffer[1], pixelsToWriteNow);
                updateAndEnqueueBufferTransaction(1, renderer, pixel, pixelsToWriteNow);
                pixel+=pixelsToWriteNow;
            }
            ESP_LOGI(TAG, "All transactions have been enqueued");
            return ErrorCode::OK;
        }

        ErrorCode DrawAsyncAsSync(AsyncRenderer *renderer)
        {
            Point2D start;
            Point2D end_ex;
            if(!renderer->GetNextOverallLimits(start, end_ex)){
                return ErrorCode::GENERIC_ERROR;
            }
            if (start.x >= end_ex.x || start.y >= end_ex.y)
                return ErrorCode::GENERIC_ERROR;
            uint32_t pixelsMax = start.PixelsTo(end_ex);
            ESP_LOGI(TAG, "Called DrawAsync for %d/%d - %d/%d with %d Pixels", start.x, start.y, end_ex.x, end_ex.y, pixelsMax);
            updateAndEnqueuePrepareTransactions(start, end_ex);
            ESP_LOGI(TAG, "Five basic transaction have been enqueued. Waiting for completion");
            for (int ti = 0; ti < 5; ti++)
            {
                spi_transaction_t* trans_desc{nullptr};
                ESP_ERROR_CHECK(spi_device_get_trans_result(spi, &trans_desc, portMAX_DELAY));
            }
            ESP_LOGI(TAG, "Five basic transaction have been completed");
            uint32_t pixel{0};
            while(pixel<pixelsMax){
                uint32_t pixelsToWriteNow = std::min(pixelsMax-pixel, PIXEL_BUFFER_SIZE_IN_PIXELS);
                if(pixelsToWriteNow>0){
                    renderer->Render(pixel, buffer[0], pixelsToWriteNow);
                    ESP_LOGI(TAG, "Buffer0 transaction will be enqueued with %d pixels. Waiting for completion", pixelsToWriteNow);
                    updateAndEnqueueBufferTransaction(0, renderer, pixel, pixelsToWriteNow);
                    spi_transaction_t* trans_desc{nullptr};
                    ESP_ERROR_CHECK(spi_device_get_trans_result(spi, &trans_desc, portMAX_DELAY));
                    ESP_LOGI(TAG, "Buffer0 transaction has been completed");
                    pixel+=pixelsToWriteNow;
                }
                pixelsToWriteNow = std::min(pixelsMax-pixel, PIXEL_BUFFER_SIZE_IN_PIXELS);
                if(pixelsToWriteNow>0){
                    renderer->Render(pixel, buffer[1], pixelsToWriteNow);
                    updateAndEnqueueBufferTransaction(1, renderer, pixel, pixelsToWriteNow);
                    ESP_LOGI(TAG, "Buffer1 transaction will be enqueued with %d pixels. Waiting for completion", pixelsToWriteNow);
                    spi_transaction_t* trans_desc{nullptr};
                    ESP_ERROR_CHECK(spi_device_get_trans_result(spi, &trans_desc, portMAX_DELAY));
                    ESP_LOGI(TAG, "Buffer1 transaction has been completed");
                    pixel+=pixelsToWriteNow;
                }
            }
            ESP_LOGI(TAG, "All transactions have been enqueued");
            return ErrorCode::OK;
        }

        static void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
        {
            SPILCD16::TransactionInfo *ti = (SPILCD16::TransactionInfo *)t->user;
            if(!ti) return;
            ti->manager->preCb(t, ti);
        }
        static void lcd_spi_post_transfer_callback(spi_transaction_t *t)
        {
            SPILCD16::TransactionInfo *ti = (SPILCD16::TransactionInfo *)t->user;
            if(!ti) return;
            ti->manager->postCb(t, ti);
        }
    };

};

#undef TAG