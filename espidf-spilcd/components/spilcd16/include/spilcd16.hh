#pragma once

#include <cstdio>
#include <cstdlib>
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
#include "./interfaces.hh"
#include "RGB565.hh"
#include "errorcodes.hh"
#include "common-esp32.hh"
#include <bit>
#include "lvgl/lvgl.h"
#include "text_utils.hh"
#define TAG "LCD"

/*
Idee: Den Auftrag, einen bestimmten Bereich des Displays neu zu befüllen, wird asynchron über einen AsyncRenderer erledigt.
Dieser wird in der sechsten Phase der Transaktion aktiv und übernimmt dann
Achtung. Wenn der Renderer mal false zurück liefert, müssen die Datenstrukturen auf dem Heap wieder entfernt werden, also insbesondere der Renderer und die Buffer

*/

#define LCD135x240 135, 240, 52, 40

namespace SPILCD16
{

    constexpr size_t PIXEL_BUFFER_SIZE_IN_PIXELS = 240 * 8; // Wenn die volle Breite ausgenutzt würde, könnten 16 Zeilen "auf einen Rutsch" übertragen werden
    constexpr int SPI_Command_Mode = 0;
    constexpr int SPI_Data_Mode = 1;
    constexpr bool POST_TRANSACTION_CALLBACK_ACTIVE{false};

    namespace CMD
    {
        constexpr uint8_t NOP = 0x00;
        constexpr uint8_t Software_Reset = 0x01;
        constexpr uint8_t RDDID = 0x04;
        constexpr uint8_t RDDST = 0x09;

        constexpr uint8_t RDDPM = 0x0A;      // Read display power mode
        constexpr uint8_t RDD_MADCTL = 0x0B; // Read display MADCTL
        constexpr uint8_t RDD_COLMOD = 0x0C; // Read display pixel format
        constexpr uint8_t RDDIM = 0x0D;      // Read display image mode
        constexpr uint8_t RDDSM = 0x0E;      // Read display signal mode
        constexpr uint8_t RDDSR = 0x0F;      // Read display self-diagnostic result (ST7789V)

        constexpr uint8_t Sleep_In = 0x10;
        constexpr uint8_t Sleep_Out = 0x11;
        constexpr uint8_t Partial_Display_Mode_On = 0x12;
        constexpr uint8_t Normal_Display_Mode_On = 0x13; //"Normal" is the opposite of "Partitial"

        constexpr uint8_t DisplayInversion_Off = 0x20;
        constexpr uint8_t Display_Inversion_On = 0x21;
        constexpr uint8_t Gamma_Set = 0x26;
        constexpr uint8_t Display_Off = 0x28;
        constexpr uint8_t Display_On = 0x29;
        constexpr uint8_t Column_Address_Set = 0x2A;
        constexpr uint8_t Row_Address_Set = 0x2B;
        constexpr uint8_t Memory_Write = 0x2C;
        constexpr uint8_t RGBSET = 0x2D; // Color setting for 4096, 64K and 262K colors
        constexpr uint8_t RAMRD = 0x2E;

        constexpr uint8_t PTLAR = 0x30;
        constexpr uint8_t VSCRDEF = 0x33;                    // Vertical scrolling definition (ST7789V)
        constexpr uint8_t TEOFF = 0x34;                      // Tearing effect line off
        constexpr uint8_t TEON = 0x35;                       // Tearing effect line on
        constexpr uint8_t Memory_Data_Access_Control = 0x36; // Memory data access control
        constexpr uint8_t IDMOFF = 0x38;                     // Idle mode off
        constexpr uint8_t IDMON = 0x39;                      // Idle mode on
        constexpr uint8_t RAMWRC = 0x3C;                     // Memory write continue (ST7789V)
        constexpr uint8_t RAMRDC = 0x3E;                     // Memory read continue (ST7789V)
        constexpr uint8_t Interface_Pixel_Format = 0x3A;

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

    enum class TransactionPhase
    {
        DAT_WRITE_B0 = 0, // 0 und 1 sind Buffer-Operationen. Ansonsten: gerade Zahlen sind Commands, ungerade Zahlen sind Data
        DAT_WRITE_B1 = 1,
        CMD_CA_SET = 2,
        DAT_CA_SET = 3,
        CMD_RA_SET = 4,
        DAT_RA_SET = 5,
        CMD_WRITE = 6,
        SYNC_DAT = 7,
        SYNC_CMD = 8,
    };

    class FilledRectRenderer : public IAsyncRenderer
    {
    private:
        Point2D start;
        Point2D end_excl;
        uint16_t foreground;
        bool getOverallLimitsCalled{false};

    public:
        FilledRectRenderer(Point2D start, Point2D end_excl, Color::Color565 foreground) : start(start), end_excl(end_excl), foreground(foreground.toST7789_SPI_native()) {}

        bool GetNextOverallLimits(size_t bufferSize, Point2D &start, Point2D &end_excl) override
        {
            if (getOverallLimitsCalled)
                return false;
            start.CopyFrom(this->start);
            end_excl.CopyFrom(this->end_excl);
            getOverallLimitsCalled = true;
            return true;
        }

        void Render(uint16_t startline, uint16_t linesCount, uint16_t *buffer) override
        {
            
            for (int i = 0; i < (end_excl.x-start.x)*linesCount; i++)
            {
                buffer[i] = this->foreground;
            }
        }
    };

           

     
    class TextRenderer : public IAsyncRenderer
    {
    private:
        const lv_font_t *font;
        Point2D pos;
        uint16_t colors[16];
        char *text;
        char *c;
        uint32_t currentGlyphIndex{0};
        uint32_t nextGlyphIndex{0};
        

    public:
        TextRenderer(const lv_font_t *font, Point2D start, Color::Color565 foreground, Color::Color565 background, char *text_will_be_freeed_inside) : font(font), pos(start), text(text_will_be_freeed_inside), c(text_will_be_freeed_inside) {
            for(int i=0;i<16;i++){
                colors[i]=background.overlayWith(foreground, opa4_table[i]).toST7789_SPI_native();
            }
            if (!c) {
                return;
            }
            if(!(*c)){
                free(text);
                return;
            }
            nextGlyphIndex= GetGlyphIndex(getCodepointAndAdvancePointer(&c)); 
        }

        bool GetNextOverallLimits(size_t bufferSize, Point2D &start, Point2D &end_excl) override
        {
            if(!nextGlyphIndex){
                free(text);
                c=nullptr;
                return false;
            }
            currentGlyphIndex=nextGlyphIndex;
            nextGlyphIndex= GetGlyphIndex(getCodepointAndAdvancePointer(&c));
            int32_t kv = GetKerningValue(currentGlyphIndex, nextGlyphIndex); 
            
            const lv_font_fmt_txt_glyph_dsc_t * glyph_dsc= GetGlyphDesc(currentGlyphIndex);
            start.x = pos.x + glyph_dsc->ofs_x;
            end_excl.x = start.x + glyph_dsc->box_w;
            end_excl.y = pos.y + glyph_dsc->ofs_y;
            start.y = end_excl.y - glyph_dsc->box_h;

            pos.x += ((glyph_dsc->adv_w+kv) + (1 << 3)) >> 4;
            ESP_LOGI(TAG, "Write GlyphIndex %ld from (%d/%d) to (%d/%d), kv=%li, next x=%d",currentGlyphIndex, start.x, start.y, end_excl.x, end_excl.y, kv, pos.x);
     
            return true;
        }

        const lv_font_fmt_txt_glyph_dsc_t * GetGlyphDesc(uint32_t glyphIndex)
        {
            return font->dsc->glyph_dsc + glyphIndex;
        }

        uint32_t GetGlyphIndex(uint32_t codepoint)
        {
            if(!codepoint) return 0; //fail fast
            for (int mapIndex = 0; mapIndex < font->dsc->cmap_num; mapIndex++)
            {
                lv_font_fmt_txt_cmap_t map = font->dsc->cmaps[mapIndex];
                if ((codepoint < map.range_start) || (map.range_start + map.range_length <= codepoint))
                {
                    continue;
                }
                switch (map.type)
                {
                case LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY:
                    return map.glyph_id_start + codepoint - map.range_start;
                case LV_FONT_FMT_TXT_CMAP_SPARSE_TINY:
                    for (int offset = 0; offset < map.list_length; offset++)
                    {
                        if (map.unicode_list[offset] == codepoint)
                        {
                            return map.glyph_id_start + offset;
                        }
                    }
                    break;
                default:
                    break;
                }
                break;
            }
            return 0;
        }

        int32_t GetKerningValue(uint32_t gid_left, uint32_t gid_right)
        {
            assert(gid_left);
            if(!gid_right) return 0;
            const lv_font_fmt_txt_dsc_t *fdsc = font->dsc;

            int8_t value = 0;

            if (fdsc->kern_classes == 0)
            {
                ESP_LOGW(TAG, "fdsc->kern_classes == 0 not supported");
             
            }
            else if (fdsc->kern_classes == 1 && fdsc->kern_dsc)
            {
                /*Kern classes*/
                const lv_font_fmt_txt_kern_classes_t *kdsc = static_cast<const lv_font_fmt_txt_kern_classes_t *>(fdsc->kern_dsc);
                
                uint8_t left_class = kdsc->left_class_mapping[gid_left];
                uint8_t right_class = kdsc->right_class_mapping[gid_right];

                /*If class = 0, kerning not exist for that glyph
                 *else got the value form `class_pair_values` 2D array*/
                if (left_class > 0 && right_class > 0)
                {
                    value = kdsc->class_pair_values[(left_class - 1) * kdsc->right_class_cnt + (right_class - 1)];
                }
            }
            return  (value * font->dsc->kern_scale) >> 4;//kernscale==16, dann wieder durch 16 teilen, also keine Änderung
        }

        void Render(uint16_t startLine, uint16_t numberOfLines, uint16_t *buffer) override
        {
            const lv_font_fmt_txt_glyph_dsc_t * glyph_dsc= GetGlyphDesc(currentGlyphIndex);
            if (glyph_dsc->box_h != numberOfLines)
            {
                ESP_LOGE(TAG, "Expected len=%u, buffer len=%u", glyph_dsc->box_h, numberOfLines);
            }
            const uint8_t *bitmap = font->dsc->glyph_bitmap;
            uint32_t bo = glyph_dsc->bitmap_index;
            uint8_t bits = 0, bit = 0;
            size_t i = 0;
            if (font->dsc->bpp == 4)
            {
                // immer zwei pixel in einem rutsch
                while (i < glyph_dsc->box_w*numberOfLines)
                {
                    bits = bitmap[bo++];
                    buffer[i++] = colors[(bits & 0xF0) >> 4];
                    if (i < glyph_dsc->box_w*numberOfLines)
                    {
                        buffer[i++] = colors[bits & 0x0F];
                    }
                }
            }
            else if (font->dsc->bpp == 1)
            {
                for (size_t yy = 0; yy < glyph_dsc->box_h; yy++)
                {
                    for (size_t xx = 0; xx < glyph_dsc->box_w; xx++)
                    {
                        if (!(bit++ & 7))
                        {
                            bits = bitmap[bo++];
                        }
                        buffer[i] = bits & 0x80 ? colors[15] : colors[0];
                        i++;
                        bits <<= 1;
                    }
                }
            }
        }
    };
    
    class TransactionInfo;

    class ITransactionCallback
    {
    public:
        virtual void preCb(spi_transaction_t *t, TransactionInfo *) = 0;
        virtual void postCb(spi_transaction_t *t, TransactionInfo *) = 0;
    };

    class TransactionInfo
    {
    public:
        ITransactionCallback *manager;
        TransactionPhase phase;
        IAsyncRenderer *renderer;
        uint16_t startLine; // max value =240*320=76.800, which is too large for uint16_t
        uint16_t numberOfPixelInOneLine;
        uint16_t numberOfLinesTotal;
        uint16_t numberOfLinesPerCall;
        TransactionInfo(ITransactionCallback *manager, TransactionPhase phase) : manager(manager), phase(phase) {}
        void ResetTo(IAsyncRenderer *renderer,  uint16_t startLine,uint16_t numberOfPixelInOneLine,uint16_t numberOfLinesTotal, uint16_t numberOfLinesPerCall)
        {
            this->renderer = renderer;
            this->startLine = startLine;
            this->numberOfPixelInOneLine = numberOfPixelInOneLine;
            this->numberOfLinesTotal=numberOfLinesTotal;
            this->numberOfLinesPerCall = numberOfLinesPerCall;
        }
    };

    template <spi_host_device_t spiHost, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs, gpio_num_t dc, gpio_num_t rst, gpio_num_t bl, uint16_t WIDTH, uint16_t HEIGHT, int16_t OFFSET_X, int16_t OFFSET_Y>
    class M : public ITransactionCallback
    {
    private:
        spi_device_handle_t spi;
        uint16_t *buffer[2];
        spi_transaction_t PREPARE_TRANSACTIONS[5];
        spi_transaction_t BUFFER_TRANSACTIONS[2];
        spi_transaction_t fooTransaction;
        SemaphoreHandle_t xSemaphore;

        // For Debugging purposes
        int preCbCalls = 0;
        int postCb1Calls = 0;
        int postCb2Calls = 0;

        void prepareTransactionAndSend(const uint8_t *data, size_t len_in_bytes)
        {
            fooTransaction.length = len_in_bytes * 8;
            fooTransaction.tx_buffer = data;
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &fooTransaction));
            return;
        }

        void lcd_cmd(const uint8_t cmd)
        {
            gpio_set_level(dc, SPI_Command_Mode);
            prepareTransactionAndSend(&cmd, 1);
        }

        void lcd_data(const uint8_t data)
        {
            gpio_set_level(dc, SPI_Data_Mode);
            prepareTransactionAndSend(&data, 1);
        }

        void updateAndEnqueuePrepareTransactions(Point2D start, Point2D end_ex, bool considerOffsetsOfVisibleArea = true, bool polling = false)
        {
            // Datasheet says: The address ranges are X=0 to X=239 (Efh) and Y=0 to Y=319 (13Fh).
            // For example the whole display contents will be written [...]: XS=0 (0h) YS=0 (0h) and XE=239 (Efh), YE=319 (13Fh).
            // THAT MEANS: end is inclusive!
            // As the end point is given exclusive, then we have to subtract 1
            // Furthermore we have to consider all offsets
            int16_t startX = start.x + (considerOffsetsOfVisibleArea ? OFFSET_X : 0);
            int16_t endX = end_ex.x - 1 + (considerOffsetsOfVisibleArea ? OFFSET_X : 0);
            int16_t startY = start.y + (considerOffsetsOfVisibleArea ? OFFSET_Y : 0);
            int16_t endY = end_ex.y - 1 + (considerOffsetsOfVisibleArea ? OFFSET_Y : 0);
            // Casting des Buffers auf int16* und dann direktes Schreiben geht nicht, weil weil little endian, also niederwertiges byte zuerst. Hier wird höherwertiges byte zuerst verlangt
            PREPARE_TRANSACTIONS[1].tx_data[0] = (startX) >> 8;   // Start Col High
            PREPARE_TRANSACTIONS[1].tx_data[1] = (startX) & 0xff; // Start Col Low
            PREPARE_TRANSACTIONS[1].tx_data[2] = (endX) >> 8;     // End Col High
            PREPARE_TRANSACTIONS[1].tx_data[3] = (endX) & 0xff;   // End Col Low
            PREPARE_TRANSACTIONS[3].tx_data[0] = (startY) >> 8;   // Start Col High
            PREPARE_TRANSACTIONS[3].tx_data[1] = (startY) & 0xff; // Start Col Low
            PREPARE_TRANSACTIONS[3].tx_data[2] = (endY) >> 8;     // End Col High
            PREPARE_TRANSACTIONS[3].tx_data[3] = (endY) & 0xff;   // End Col Low
            for (int ti = 0; ti < 5; ti++)
            {
                if (polling)
                    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &PREPARE_TRANSACTIONS[ti]));
                else
                    ESP_ERROR_CHECK(spi_device_queue_trans(spi, &PREPARE_TRANSACTIONS[ti], pdMS_TO_TICKS(100)));
            }
        }

        void updateAndEnqueueBufferTransaction(int index, IAsyncRenderer *renderer, uint16_t startline, uint16_t lineLengthPixel, uint16_t linesCountTotal, uint16_t linesCountInOneCall, bool polling = false)
        {
            index = index % 2;
            BUFFER_TRANSACTIONS[index].length = 16 * lineLengthPixel*linesCountInOneCall; // Data length, in bits
            BUFFER_TRANSACTIONS[index].rxlength = 0;
            static_cast<TransactionInfo *>(BUFFER_TRANSACTIONS[index].user)->ResetTo(renderer, startline, lineLengthPixel, linesCountTotal, linesCountInOneCall);
            if (linesCountInOneCall > 0)
            {
                if (polling)
                    ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &BUFFER_TRANSACTIONS[index]));
                else
                    ESP_ERROR_CHECK(spi_device_queue_trans(spi, &BUFFER_TRANSACTIONS[index], pdMS_TO_TICKS(100)));
            }
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
            memset(PREPARE_TRANSACTIONS, 0, 5 * sizeof(spi_transaction_t));

            PREPARE_TRANSACTIONS[0].length = 8; // bits
            PREPARE_TRANSACTIONS[0].user = (void *)new TransactionInfo(this, TransactionPhase::CMD_CA_SET);
            PREPARE_TRANSACTIONS[0].tx_data[0] = CMD::Column_Address_Set;
            PREPARE_TRANSACTIONS[0].flags = SPI_TRANS_USE_TXDATA; // means: data is not in a buffer, that is pointed with tx_buffer, but it is directly in tx_data

            PREPARE_TRANSACTIONS[1].length = 8 * 4; // bits
            PREPARE_TRANSACTIONS[1].user = (void *)new TransactionInfo(this, TransactionPhase::DAT_CA_SET);
            PREPARE_TRANSACTIONS[1].flags = SPI_TRANS_USE_TXDATA;

            PREPARE_TRANSACTIONS[2].length = 8; // bits
            PREPARE_TRANSACTIONS[2].user = (void *)new TransactionInfo(this, TransactionPhase::CMD_RA_SET);
            PREPARE_TRANSACTIONS[2].tx_data[0] = CMD::Row_Address_Set;
            PREPARE_TRANSACTIONS[2].flags = SPI_TRANS_USE_TXDATA;

            PREPARE_TRANSACTIONS[3].length = 8 * 4; // bits
            PREPARE_TRANSACTIONS[3].user = (void *)new TransactionInfo(this, TransactionPhase::DAT_RA_SET);
            PREPARE_TRANSACTIONS[3].flags = SPI_TRANS_USE_TXDATA;

            PREPARE_TRANSACTIONS[4].length = 8; // bits
            PREPARE_TRANSACTIONS[4].user = (void *)new TransactionInfo(this, TransactionPhase::CMD_WRITE);
            PREPARE_TRANSACTIONS[4].tx_data[0] = CMD::Memory_Write; // memory write
            PREPARE_TRANSACTIONS[4].flags = SPI_TRANS_USE_TXDATA;

            memset(BUFFER_TRANSACTIONS, 0, 2 * sizeof(spi_transaction_t));
            BUFFER_TRANSACTIONS[0].tx_buffer = buffer[0];
            BUFFER_TRANSACTIONS[0].user = (void *)new TransactionInfo(this, TransactionPhase::DAT_WRITE_B0);
            BUFFER_TRANSACTIONS[1].tx_buffer = buffer[1];
            BUFFER_TRANSACTIONS[1].user = (void *)new TransactionInfo(this, TransactionPhase::DAT_WRITE_B1);

            memset(&fooTransaction, 0, sizeof(spi_transaction_t));
        }

        void preCb(spi_transaction_t *t, TransactionInfo *ti)
        {
            // größer 1 und gerade ist CMD
            if ((int)(ti->phase) > 1 && (int)(ti->phase) % 2 == 0)
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
            if (!POST_TRANSACTION_CALLBACK_ACTIVE)
                return;
            if (1 < (int)ti->phase)
            {
                // Eine Vorbereitungs-Transaktion oder eine "SYNC"-Transaktion ist fertig - das interessiert uns hier nicht
                postCb2Calls++;
                return;
            }
            TransactionInfo *other_ti = static_cast<TransactionInfo *>(BUFFER_TRANSACTIONS[(int)(ti->phase) == 0 ? 1 : 0].user);
            if (other_ti->startLine + other_ti->numberOfLinesPerCall == ti->numberOfLinesTotal)
            {
                // die andere Transaktion wird gleich dafür sorgen, dass dass Rect fertig übertragen wird - diese Transaktion ist also der vorletzte Schritt
                // nichts machen, sonst gerät alles durcheinander, warte, bis die andere fertig ist und versuche dann, zum nächsten Rect zu gehen (s.u.)
                return;
            }
            IAsyncRenderer *renderer = ti->renderer;
            if (ti->startLine + ti->numberOfLinesPerCall == ti->numberOfLinesTotal)
            {
                // diese Transaktion sorgte dafür, dass dass Rect fertig übertragen wird - diese Transaktion ist also der LETZTE Schritt
                // wir versuchen, zum nächsten Rect zu wechseln
                Point2D start;
                Point2D end_ex;
                if (!renderer->GetNextOverallLimits(PIXEL_BUFFER_SIZE_IN_PIXELS, start, end_ex))
                {
                    // Es gibt kein weiteres Rect --> wir müssen keine neue Datentransaktion auflegen
                    // wir sind komplett fertig!!!!
                    xSemaphoreGive(xSemaphore);
                    return;
                }
                // Es gibt ein weiteres Rect -->initialisierung
                if (start.x >= end_ex.x || start.y >= end_ex.y)
                {
                    // unsinnige Koordinaten des Rechtecks --> auch einfach aufhören
                    return;
                }
                updateAndEnqueuePrepareTransactions(start, end_ex); //
                fillBothBuffersWithNewRect(renderer, end_ex.y-start.y, end_ex.x-start.x, false);
                return;
            }

            int nextPhase = (int)ti->phase == 0 ? 1 : 0;
            uint16_t *nextBuf = this->buffer[(int)ti->phase == 0 ? 1 : 0];
            postCb1Calls++;

            uint16_t nextStartLine{(uint16_t)(ti->startLine + ti->numberOfLinesPerCall)};
            uint32_t nextNumberOfLinesPerCall = std::min((uint16_t)(ti->numberOfLinesTotal - nextStartLine), (uint16_t)(PIXEL_BUFFER_SIZE_IN_PIXELS/ti->numberOfPixelInOneLine));
            assert(nextNumberOfLinesPerCall > 0); // Denn ansonsten hätte das oben schon erkannt werden müssen

            renderer->Render(nextStartLine, nextNumberOfLinesPerCall, nextBuf);
            updateAndEnqueueBufferTransaction(nextPhase, renderer, nextStartLine, ti->numberOfPixelInOneLine, ti->numberOfLinesTotal, nextNumberOfLinesPerCall);
        }

        void Loop()
        {
            ESP_LOGI(TAG, "preCb %d, postCb1 %d, postCb2 %d", this->preCbCalls, postCb1Calls, postCb2Calls);
        }

        esp_err_t InitSpiAndGpio()
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
                delayMs(100);
                gpio_set_level(rst, 1);
                delayMs(100);
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
            return ESP_OK;
        }

        void Init_ST7789(Color::Color565 fillColor = Color::BLACK)
        {

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

            lcd_cmd(CMD::Software_Reset);
            // If software reset is sent during sleep in mode, it will be necessary to wait 120msec before sending sleep out command.
            delayAtLeastMs(150);

            lcd_cmd(CMD::Sleep_Out);
            // It will be necessary to wait 5msec before sending any new commands to a display module following this command to allow time for the supply voltages and clock circuits to stabilize.
            delayAtLeastMs(10);

            lcd_cmd(CMD::Interface_Pixel_Format);
            lcd_data(0x05); // 16bit color format

            lcd_cmd(CMD::Memory_Data_Access_Control); // necessary, because not all types of reset set this value to 0x00
            lcd_data(0x00);

            lcd_cmd(CMD::Display_Inversion_On); // enable the color inversion mode that is necessary for in-plane switching (IPS) TFT displays.

            lcd_cmd(CMD::Display_On);

            FilledRectRenderer rr(Point2D(0, 0), Point2D(135, 240), fillColor);
            DrawAsyncAsSync(&rr, true);
            // FillRectSyncPolling(Point2D(0, 0), Point2D(135, 240), fillColor);
            // FillRectSyncPolling(Point2D(0, 10), Point2D(5, 230), RGB565::RED);//links
            // FillRectSyncPolling(Point2D(130, 10), Point2D(135, 230), RGB565::RED);//rechts
            // FillRectSyncPolling(Point2D(10, 0), Point2D(125, 5), RGB565::GREEN);//oben
            // FillRectSyncPolling(Point2D(10, 235), Point2D(125, 240), RGB565::GREEN);//unten
        }


        ErrorCode FillRectSyncPolling(Point2D start, Point2D end_ex, Color::Color565 col, bool considerOffsetsOfVisibleArea = true)
        {

            if (start.x >= end_ex.x || start.y >= end_ex.y)
                return ErrorCode::GENERIC_ERROR;
            size_t pixels = (end_ex.x - start.x) * (end_ex.y - start.y);
            ESP_LOGI(TAG, "Called FillRect for %d/%d - %d/%d, aka %d pixels of %d", start.x, start.y, end_ex.x, end_ex.y, pixels, PIXEL_BUFFER_SIZE_IN_PIXELS);
            int16_t startX = start.x + (considerOffsetsOfVisibleArea ? OFFSET_X : 0);
            int16_t endX = end_ex.x - 1 + (considerOffsetsOfVisibleArea ? OFFSET_X : 0);
            int16_t startY = start.y + (considerOffsetsOfVisibleArea ? OFFSET_Y : 0);
            int16_t endY = end_ex.y - 1 + (considerOffsetsOfVisibleArea ? OFFSET_Y : 0);
            spi_transaction_t trans = {};
            memset(&trans, 0, sizeof(spi_transaction_t));
            trans.length = 8;
            trans.user = nullptr;
            trans.tx_data[0] = CMD::Column_Address_Set; // Column Address Set
            trans.flags = SPI_TRANS_USE_TXDATA;
            gpio_set_level(dc, SPI_Command_Mode);
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8 * 4;
            trans.user = nullptr;
            trans.tx_data[0] = (startX) >> 8;   // Start Col High
            trans.tx_data[1] = (startX) & 0xff; // Start Col Low
            trans.tx_data[2] = (endX) >> 8;     // End Col High
            trans.tx_data[3] = (endX) & 0xff;   // End Col Low
            trans.flags = SPI_TRANS_USE_TXDATA;
            gpio_set_level(dc, SPI_Data_Mode);
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8;
            trans.user = nullptr;
            trans.tx_data[0] = CMD::Row_Address_Set; // 0x2B; // Row Address Set
            trans.flags = SPI_TRANS_USE_TXDATA;
            gpio_set_level(dc, SPI_Command_Mode);
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8 * 4;
            trans.user = nullptr;
            trans.tx_data[0] = (startY) >> 8;   // Start page High
            trans.tx_data[1] = (startY) & 0xff; // Start page Low
            trans.tx_data[2] = (endY) >> 8;     // End page High
            trans.tx_data[3] = (endY) & 0xff;   // End page Low
            trans.flags = SPI_TRANS_USE_TXDATA;
            gpio_set_level(dc, SPI_Data_Mode);
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));

            trans.length = 8;
            trans.user = nullptr;
            trans.tx_data[0] = CMD::Memory_Write; // memory write
            trans.flags = SPI_TRANS_USE_TXDATA;
            gpio_set_level(dc, SPI_Command_Mode);
            ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));
            uint16_t hw_col = col.toST7789_SPI_native();
            for (int i = 0; i < PIXEL_BUFFER_SIZE_IN_PIXELS; i++)
            {
                buffer[0][i] = hw_col;
            }

            trans.user = nullptr;
            trans.tx_buffer = buffer[0]; // finally send the line data
            trans.flags = 0;
            gpio_set_level(dc, SPI_Data_Mode);
            while (pixels > 0)
            {
                size_t pixelsNow = std::min(PIXEL_BUFFER_SIZE_IN_PIXELS, pixels);
                trans.length = 16 * pixelsNow; // Data length, in bits
                ESP_ERROR_CHECK(spi_device_polling_transmit(spi, &trans));
                pixels -= pixelsNow;
            }
            ESP_LOGI(TAG, "FillRect finished");
            return ErrorCode::OK;
        }

        void fillBothBuffersWithNewRect(IAsyncRenderer *renderer, uint16_t linesTotal, uint16_t lineLengthPixel, bool logoutputEnabled)
        {
            uint16_t startLine{0};
            uint16_t linesInOneCall{0};
            linesInOneCall = std::min((uint16_t)(linesTotal - startLine), (uint16_t)(PIXEL_BUFFER_SIZE_IN_PIXELS/lineLengthPixel));
            if (linesInOneCall > 0)
            {
                renderer->Render(startLine, linesInOneCall, buffer[0]);
                if (logoutputEnabled)
                    ESP_LOGI(TAG, "Buffer0 transaction will be enqueued with %u lines. ", linesInOneCall);
                updateAndEnqueueBufferTransaction(0, renderer, startLine, lineLengthPixel, linesTotal, linesInOneCall);
                startLine += linesInOneCall;
            }

            linesInOneCall = std::min((uint16_t)(linesTotal - startLine), (uint16_t)(PIXEL_BUFFER_SIZE_IN_PIXELS/lineLengthPixel));
            if (linesInOneCall > 0)
            {
                renderer->Render(startLine, linesInOneCall, buffer[1]);
                if (logoutputEnabled)
                    ESP_LOGI(TAG, "Buffer1 transaction will be enqueued with %u lines. ", linesInOneCall);
                updateAndEnqueueBufferTransaction(1, renderer, startLine, lineLengthPixel, linesTotal, linesInOneCall);
                startLine += linesInOneCall;
            }
            else
            {
                updateAndEnqueueBufferTransaction(1, renderer, startLine, lineLengthPixel, linesTotal, 0);
            }
        }

        ErrorCode DrawAsync(IAsyncRenderer *renderer)
        {

            xSemaphoreTake(xSemaphore, portMAX_DELAY);
            Point2D start;
            Point2D end_ex;
            if (!renderer->GetNextOverallLimits(PIXEL_BUFFER_SIZE_IN_PIXELS, start, end_ex))
            {
                // es gibt nicht mal ein erstes Rechteck vom Renderer - das kann doch nicht sein!
                return ErrorCode::GENERIC_ERROR;
            }
            if (start.x >= end_ex.x || start.y >= end_ex.y)
                return ErrorCode::GENERIC_ERROR;
            uint32_t pixelsOfRect = start.PixelsTo(end_ex);
            ESP_LOGI(TAG, "Called DrawAsync for (%d/%d) - (%d/%d) with %d Pixels", start.x, start.y, end_ex.x, end_ex.y, pixelsOfRect);
            updateAndEnqueuePrepareTransactions(start, end_ex);
            ESP_LOGI(TAG, "Five basic transaction have been enqueued.");
            fillBothBuffersWithNewRect(renderer, pixelsOfRect, true);

            ESP_LOGI(TAG, "All initial transactions have been enqueued");
            return ErrorCode::OK;
        }

        ErrorCode DrawAsyncAsSync(IAsyncRenderer *renderer, bool considerOffsetsOfVisibleArea = true)
        {
            Point2D start;
            Point2D end_ex;
            while (renderer->GetNextOverallLimits(PIXEL_BUFFER_SIZE_IN_PIXELS, start, end_ex))
            {

                if (start.x >= end_ex.x || start.y >= end_ex.y)
                    return ErrorCode::GENERIC_ERROR;
                uint16_t linesTotal = end_ex.y-start.y;
                uint16_t lineLengthPixel = end_ex.x-start.x;
                ESP_LOGI(TAG, "Called DrawAsyncAsSync for %d/%d - %d/%d with %u lines", start.x, start.y, end_ex.x, end_ex.y, linesTotal);
                updateAndEnqueuePrepareTransactions(start, end_ex, considerOffsetsOfVisibleArea, true);
                ESP_LOGI(TAG, "Five basic transaction in DrawAsyncAsSync have been completed");
                uint16_t startLine{0};
                while (startLine < linesTotal)
                {
                    uint16_t linesInCall = std::min((uint16_t)(linesTotal - startLine), (uint16_t)(PIXEL_BUFFER_SIZE_IN_PIXELS/lineLengthPixel));
                    if (linesInCall > 0)
                    {
                        renderer->Render(startLine, linesInCall, buffer[0]);
                        updateAndEnqueueBufferTransaction(0, renderer, startLine, lineLengthPixel, linesTotal, linesInCall, true);
                        ESP_LOGI(TAG, "Buffer0 transaction in DrawAsyncAsSync has been completed with %u lines.", linesInCall);
                        startLine += linesInCall;
                    }
                    delayMs(100);
                    linesInCall = std::min((uint16_t)(linesTotal - startLine), (uint16_t)(PIXEL_BUFFER_SIZE_IN_PIXELS/lineLengthPixel));
                    if (linesInCall > 0)
                    {
                        renderer->Render(startLine, linesInCall, buffer[1]);
                        updateAndEnqueueBufferTransaction(1, renderer, startLine, lineLengthPixel, linesTotal, linesInCall, true);
                        ESP_LOGI(TAG, "Buffer1 transaction in DrawAsyncAsSync has been completed with %u lines.", linesInCall);
                        startLine += linesInCall;
                    }
                    delayMs(100);
                }
            }
            ESP_LOGI(TAG, "All transactions have been enqueued");
            return ErrorCode::OK;
        }

        size_t printfl(int line, bool invert, const char *format, ...){
            va_list args_list;
            va_start(args_list, format);
            char *chars{nullptr};
            int chars_len = vasprintf(&chars, format, args_list);
            va_end(args_list);
            if (chars_len <= 0)
            {
                free(chars);
                return 0;
            }
            TextRenderer tr(&Roboto_regular, Point2D(52, 100), Color::BLACK, Color::YELLOW, chars);
            DrawAsyncAsSync(&tr);
            //free(chars); not necessary, this is done inside TextRenderer
            return chars_len;
        }

        static void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
        {
            if (t->user == nullptr)
                return;
            SPILCD16::TransactionInfo *ti = (SPILCD16::TransactionInfo *)t->user;
            ti->manager->preCb(t, ti);
        }
        static void lcd_spi_post_transfer_callback(spi_transaction_t *t)
        {
            if (t->user == nullptr)
                return;
            SPILCD16::TransactionInfo *ti = (SPILCD16::TransactionInfo *)t->user;
            ti->manager->postCb(t, ti);
        }
    };

};

#undef TAG