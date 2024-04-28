#pragma once
#include <algorithm>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "string.h" // for memset
#include "klfont.hh"
#include <ssd1306_cmd.hh>
#include <interfaces.hh>
#include <stdio.h>
#include "text_utils.hh"
#include <esp_log.h>
#define TAG "LCD"



namespace ssd1306
{

    esp_err_t InitI2C(gpio_num_t scl, gpio_num_t sda, i2c_master_bus_handle_t *bus_handle)
    {
        i2c_master_bus_config_t i2c_mst_config = {};

        i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
        i2c_mst_config.i2c_port = -1;
        i2c_mst_config.scl_io_num = scl;
        i2c_mst_config.sda_io_num = sda;
        i2c_mst_config.glitch_ignore_cnt = 7;
        i2c_mst_config.flags.enable_internal_pullup = true;
        return i2c_new_master_bus(&i2c_mst_config, bus_handle);
    }

    constexpr int I2C_TICKS_TO_WAIT = 100;
    constexpr int PADDING_LEFT{3};
    constexpr int PADDING_RIGHT{3};
    constexpr int LINE_WIDTH_PIXELS{128};

    /**
     * Allways positive; for array indices!
     */
    inline size_t modulo(const int i, const int n)
    {
        return (i % n + n) % n;
    }

    struct GlyphHelper{
        const GlyphDesc* glyph_dsc;
        int16_t startX;
        /*
        p muss an der startX-Position des Bitmaps stehen
        suppressedPixelIfBackground: so viele Pixel werden im BUffer nicht überschreiben, wenn Sie nur Hintergrund sind (wichtig beim Kerning)
        */
        void WriteLineToBuffer(const FontDesc *font, bool invert, uint16_t ramline, uint8_t *buffer)
        {
            const uint8_t *bitmap= font->glyph_bitmap->begin();
            uint32_t bo = glyph_dsc->bitmap_index;
            ESP_LOGI(TAG, "bitmap_base_address =0x%08X, glyph_bitmap_offset=%lu", (unsigned int)bitmap, bo);
            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = ramline * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + indexInBitmap];  
                ESP_LOGI(TAG, "indexInBitmap=%i, bits=0x%02X", indexInBitmap, bits);
                if(!invert){
                    buffer[startX+x]|=bits;
                }else{
                    buffer[startX+x]&=~bits;
                }
            }
        }
    };

    template <uint8_t WIDTH = 128, uint8_t HEIGHT = 32, uint8_t I2C_ADDRESS = 0x3C, bool EXTERNALVCC = false, bool FLIP = false>
    class M : public display::FullLineWriter
    {

    public:
        uint8_t GetShownLines() override
        {
            return HEIGHT / (font->line_height);
        }

        uint8_t GetAvailableLines() override
        {
            return 64 / (font->line_height);
        }

        void ClearScreenAndResetStartline(bool invert = false, uint8_t start_textline_nominator = 0, uint8_t start_textline_denominator = 1)
        {

            uint8_t cmd_buf[4];
            cmd_buf[0] = CMD::WRITE_CMD_STREAM;
            // Set Lower Column Start Address for Page Addressing Mode
            cmd_buf[1] = (0x00);
            // Set Higher Column Start Address for Page Addressing Mode
            cmd_buf[2] = (0x10);

            uint8_t data_buf[WIDTH + 1];
            data_buf[0] = CMD::WRITE_DAT;
            memset(data_buf + 1, invert ? 0xFF : 0x00, WIDTH);

            for (int line = 0; line < 8; line++)
            {
                // Set Page Start Address for Page Addressing Mode
                cmd_buf[3] = 0xB0 | line;
                ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, cmd_buf, 4, I2C_TICKS_TO_WAIT));
                ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_buf, WIDTH + 1, I2C_TICKS_TO_WAIT));
            }

            SetStartline((float)(this->font->line_height * start_textline_nominator) / (float)start_textline_denominator);
        }

        void SetStartline(uint8_t startline)
        {
            this->startline = (startline + 64) % 64;
            uint8_t buf[] = {CMD::WRITE_CMD_STREAM, (uint8_t)(CMD::SETSTARTLINE | this->startline)};
            ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, buf, sizeof(buf), I2C_TICKS_TO_WAIT));
        }

        void clear_screen(bool invert = false)
        {
            uint8_t space[WIDTH];
            memset(space, invert ? 0xFF : 0x00, sizeof(space));
            for (int line = 0; line < 8; line++) // clear even hidden lines -->because of possible scrolling!
            {
                display_image(line, 0, space, WIDTH);
            }
        }

        void clear_line(uint8_t line, bool invert)
        {
            line &= 7;
            uint8_t space[WIDTH];
            memset(space, invert ? 0xFF : 0x00, sizeof(space));
            display_image(line, 0, space, WIDTH);
        }

        void display_image(uint8_t line, uint8_t seg, const uint8_t *images, uint8_t width, bool invert = false)
        {
            if (line >= 8)
                return;
            if (seg >= WIDTH)
                return;

            uint8_t columLow = seg & 0x0F;
            uint8_t columHigh = (seg >> 4) & 0x0F;

            if (FLIP)
            {
                line = ((HEIGHT / 8) - line) - 1;
            }

            uint8_t *out_buf = new uint8_t[width + 1];
            int out_index = 0;
            out_buf[out_index++] = CMD::WRITE_CMD_STREAM;
            // Set Lower Column Start Address for Page Addressing Mode
            out_buf[out_index++] = (0x00 + columLow);
            // Set Higher Column Start Address for Page Addressing Mode
            out_buf[out_index++] = (0x10 + columHigh);
            // Set Page Start Address for Page Addressing Mode
            out_buf[out_index++] = 0xB0 | line;

            esp_err_t res;
            res = i2c_master_transmit(dev_handle, out_buf, out_index, I2C_TICKS_TO_WAIT);
            if (res != ESP_OK)
                ESP_LOGE(TAG, "Could not write to device [0x%02x]: %d (%s)", I2C_ADDRESS, res, esp_err_to_name(res));

            out_buf[0] = CMD::WRITE_DAT;
            if (invert)
            {
                for (int i = 0; i < width; i++)
                {
                    out_buf[1 + i] = ~images[i];
                }
            }
            else
            {
                memcpy(&out_buf[1], images, width);
            }

            res = i2c_master_transmit(dev_handle, out_buf, width + 1, I2C_TICKS_TO_WAIT);
            if (res != ESP_OK)
                ESP_LOGE(TAG, "Could not write to device [0x%02x]: %d (%s)", I2C_ADDRESS, res, esp_err_to_name(res));
            delete[] out_buf;
        }

        void update_buf(uint8_t line, uint8_t seg, const uint8_t *image, uint8_t image_width, bool invert = false, bool clear_before_write = true)
        {
            if (line >= 8)
                return;
            if (seg >= WIDTH)
                return;

            int i = 0;
            while (seg < WIDTH && i < image_width)
            {
                uint8_t value = clear_before_write ? 0 : s_chDisplayBuffer[line * WIDTH + seg];
                value |= image[i];
                if (invert)
                {
                    value = ~value;
                }
                s_chDisplayBuffer[line * WIDTH + seg] = value;
                seg++;
                i++;
            }
        }

        void Scroll(int textlines)
        {
            int step = ((this->font->line_height) / 4) * (textlines < 0 ? -1 : +1);
            int count = 4 * abs(textlines);
            ESP_LOGI(TAG, "Scoll h=%d count=%d, step=%d, old startline=%d", this->font->line_height, count, step, startline);
            for (int i = 0; i < count; i++)
            {
                startline += step;
                startline = (startline + 64) % 64;
                uint8_t buf[] = {CMD::WRITE_CMD_STREAM, (uint8_t)(CMD::SETSTARTLINE | startline)};
                ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, buf, sizeof(buf), I2C_TICKS_TO_WAIT));
                vTaskDelay(pdMS_TO_TICKS(50));
            }
            ESP_LOGI(TAG, "New startline=%d", startline);
        }

        size_t GetStartline()
        {
            return startline;
        }

        void flush_buf(uint8_t line, uint8_t startSegment, uint8_t numberOfSegments)
        {

            if (line >= 8)
                return;
            if (startSegment >= WIDTH)
                return;

            uint8_t columLow = startSegment & 0x0F;
            uint8_t columHigh = (startSegment >> 4) & 0x0F;

            write_buf[0] = CMD::WRITE_CMD_STREAM;
            write_buf[1] = (0x00 + columLow);
            write_buf[2] = (0x10 + columHigh);
            write_buf[3] = (0xB0 | line);

            ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, write_buf, 4, I2C_TICKS_TO_WAIT));
            memset(this->write_buf, 0, 128 + 5);
            write_buf[0] = CMD::WRITE_DAT;
            numberOfSegments = std::min((uint8_t)(startSegment + numberOfSegments), WIDTH) - startSegment;

            for (uint8_t seg = 0; seg < numberOfSegments; seg++)
            {

                write_buf[1 + seg] = s_chDisplayBuffer[line * WIDTH + seg + startSegment];
            }

            ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, write_buf, numberOfSegments + 1, I2C_TICKS_TO_WAIT));
        }

        void setWindowFullPage(uint8_t page)
        {
            uint8_t *out_buf = new uint8_t[4];
            page = modulo(page, 8);
            out_buf[0] = CMD::WRITE_CMD_STREAM;
            // Set Lower Column Start Address for Page Addressing Mode
            out_buf[1] = (0x00);
            // Set Higher Column Start Address for Page Addressing Mode
            out_buf[2] = (0x10);
            // Set Page Start Address for Page Addressing Mode
            out_buf[3] = 0xB0 | page;
            ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, out_buf, 4, I2C_TICKS_TO_WAIT));
        }
        
        void PopulateGlyphs(char *chars)
        {
            uint32_t currentCodepoint = text_utils::getCodepointAndAdvancePointer(&chars);
            uint32_t currentGlyphIndex=font->GetGlyphIndex(currentCodepoint);
            uint32_t nextCodepoint{0};
            uint32_t nextGlyphIndex{0};
            //two tabs are supported. First tab is centered and center-aligned
            //second tab is right and right-aligned
            size_t glyphBeforeTabulator[2]={UINT32_MAX, UINT32_MAX};
            
            uint16_t posX = PADDING_LEFT-font->glyph_desc[currentGlyphIndex].ofs_x;
            uint16_t endX{0};
            uint8_t tabIndex=0;
            while (currentCodepoint)
            {
                nextCodepoint = text_utils::getCodepointAndAdvancePointer(&chars);
                int16_t kv;
                if(nextCodepoint=='\t' && tabIndex<2){
                    glyphBeforeTabulator[tabIndex++]=glyphs.size();
                    nextCodepoint = text_utils::getCodepointAndAdvancePointer(&chars);
                    if(nextCodepoint=='\t' && tabIndex<2){
                        glyphBeforeTabulator[tabIndex++]=glyphs.size();
                        nextCodepoint = text_utils::getCodepointAndAdvancePointer(&chars);
                        ESP_LOGD(TAG, "Two Tabs detected! pos=%d, codePointAfter=%lu", glyphs.size(), nextCodepoint);
                    }
                    else{
                        ESP_LOGD(TAG, "One Tab detected! pos=%d, codePointAfter=%lu", glyphs.size(), nextCodepoint);
                    }
                    nextGlyphIndex = font->GetGlyphIndex(nextCodepoint);
                    kv=0;
                    
                } else{
                    nextGlyphIndex = font->GetGlyphIndex(nextCodepoint);
                    kv = font->GetKerningValue(currentGlyphIndex, nextGlyphIndex);
                }
                auto dsc=&font->glyph_desc[currentGlyphIndex];
                if(currentCodepoint>0xFF){
                    ESP_LOGD(TAG, "Special Codepoint detected %lu, glyphIndex=%lu, bitmapIndex=%lu", currentCodepoint, currentGlyphIndex, dsc->bitmap_index);
                }

                GlyphHelper gh = {};
                gh.glyph_dsc = dsc;
                gh.startX = posX + gh.glyph_dsc->ofs_x;
                endX = gh.startX + gh.glyph_dsc->box_w;
                if (endX >= LINE_WIDTH_PIXELS)
                {
                    ESP_LOGW(TAG, "NOT push GlyphIndex=%lu, posX=%d endX=%d, startX=%d", currentGlyphIndex, posX, endX, gh.startX);
                    break; // Damit ist sicher gestellt, dass man bei der Ausgabe keinerlei überprüfung machen muss, ob irgendwelche Grenzen überschritten werden -->einfach glyphs zeichnen und gut!
                }else{
                    ESP_LOGI(TAG, "push GlyphIndex=%lu, posX=%d nextIndex=%lu, kv=%u, startX=%d,", currentGlyphIndex, posX, nextGlyphIndex, kv, gh.startX);
                }
                //"adv_w" und "kv" are in "font units" (see "unitsPerEm" in FontDsc). If unitsPerEm=2048, then 2048 of those units means 1Em eg. normal font size eg. 16px
                if(font->unitsPerEm==0){
                    posX += (gh.glyph_dsc->adv_w + kv);
                }else{
                    posX += ((gh.glyph_dsc->adv_w + kv) + (1 << 6)) >> 7;
                }
                
                glyphs.push_back(gh);
                currentCodepoint=nextCodepoint;
                currentGlyphIndex=nextGlyphIndex;
            }

            int i=glyphs.size()-1;
            //Erst rechtsbündigen tabluator
            int offset = LINE_WIDTH_PIXELS-PADDING_RIGHT-endX;
            if(glyphBeforeTabulator[1]!=UINT32_MAX && offset>0){
                while(i>glyphBeforeTabulator[1]){
                    glyphs.at(i).startX+=offset;
                    ESP_LOGD(TAG, "moved Glyph at pos %d to posX=%d",  i, glyphs.at(i).startX);
                    i--;
                }
            }
            
            if(glyphBeforeTabulator[0]!=UINT32_MAX && glyphBeforeTabulator[0]!=glyphBeforeTabulator[1]){
                GlyphHelper* g1 =&glyphs.at(glyphBeforeTabulator[0]+1);//""
                GlyphHelper* g2 =&glyphs.at(glyphBeforeTabulator[1]);
                
                uint16_t startOfBlock=g1->startX;
                uint16_t endOfBlock =g2->startX+g2->glyph_dsc->box_w;
                ESP_LOGD(TAG, "Two separate tabs -->we need to center some glyphs, startOfBlock=%d, endOfBlock=%d", startOfBlock, endOfBlock);
                assert(endOfBlock>startOfBlock);
                uint16_t widthOfCenteredChars=endOfBlock-startOfBlock;
                uint16_t startPos = (LINE_WIDTH_PIXELS/2)-(widthOfCenteredChars/2);
                offset=startPos-g1->startX;
                assert(offset>0);
                while(i>glyphBeforeTabulator[0]){
                    glyphs.at(i).startX+=offset;
                    ESP_LOGD(TAG, "moved Glyph at pos %d to posX=%d",  i, glyphs.at(i).startX);
                    i--;
                }

            }
            
        }


        
        size_t printfl(int line, bool invert, const char *format, ...)
        {
            va_list args_list;
            va_start(args_list, format);
            size_t ret = printfl(line, invert, format, args_list);
            va_end(args_list);
            return ret;
        }
     
        size_t printfl(int line, bool invert, const char *format, va_list args_list)
        {
            char *chars{nullptr};
            int chars_len = vasprintf(&chars, format, args_list);
            if (chars_len <= 0)
            {
                free(chars);
                return 0;
            }
            glyphs.clear();
            this->PopulateGlyphs(chars);

            uint8_t *out_buf = new uint8_t[WIDTH + 1];//+1 for CMD::WRITE_DAT
            out_buf[0] = CMD::WRITE_DAT;
            uint8_t *buf=&out_buf[1];

            for (int ramline = 0; ramline < (font->line_height) / 8; ramline++)
            {
                memset(buf, invert ? 0xFF : 0x00, WIDTH);
                setWindowFullPage(ramline + line * (font->line_height) / 8);
                int ghIndex = 0;
                while (ghIndex < glyphs.size())
                {
                    GlyphHelper *g = &glyphs[ghIndex];
                    g->WriteLineToBuffer(font, invert, ramline, buf);
                    ghIndex++;
                }
                ESP_LOG_BUFFER_HEX(TAG, out_buf, 16);
                ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, out_buf, WIDTH + 1, I2C_TICKS_TO_WAIT));
            }
            free(chars);
            delete[] out_buf;
            return chars_len;
        }

        size_t lcdprintf_doesnotworkbecauseofmissingfont(int line, uint8_t segment0_127, bool invert, const char *format, ...)
        {
            line %= 8; // important: line can be an arbitrary integer
            if (line < 0)
                line += 8;
            va_list args_list;
            va_start(args_list, format);
            char *buffer{nullptr};
            int buffer_len = vasprintf(&buffer, format, args_list);
            va_end(args_list);
            if (buffer_len <= 0)
            {
                // printf("buffer_len==0\n");
                free(buffer);
                return 0;
            }

            for (uint8_t i = 0; i < buffer_len; i++)
            {
                // display_image(line, segment0_127, font::font8x8_basic_tr[(uint8_t)buffer[i]], 8, invert);
                //TODO update_buf(line, segment0_127, font::font8x8_data[(uint8_t)buffer[i]], 8, invert);
                segment0_127 = segment0_127 + 8;
            }
            flush_buf(line, 0, 128);
            free(buffer);
            return buffer_len;
        }

        void setWindow(uint8_t col0_incl, uint8_t page0_incl, uint8_t col1_incl, uint8_t page1_incl)
        {
            uint8_t buf[] = {
                CMD::WRITE_CMD_STREAM,
                CMD::SET_COLUMN_ADDRESS_RANGE, std::clamp(col0_incl, (uint8_t)0, (uint8_t)127), std::clamp(col1_incl, (uint8_t)0, (uint8_t)127),
                CMD::SET_PAGE_ADDRESS_RANGE, std::clamp(page0_incl, (uint8_t)0, (uint8_t)63), std::clamp(page1_incl, (uint8_t)0, (uint8_t)63)};
            ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, buf, sizeof(buf), I2C_TICKS_TO_WAIT));
        }

        esp_err_t Init(i2c_master_bus_handle_t bus_handle, uint32_t scl_speed_hz = 400000)
        {
            ESP_ERROR_CHECK(i2c_master_probe(bus_handle, I2C_ADDRESS, 100));
            ESP_LOGI(TAG, "OLED SSD1306 found!");

            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = I2C_ADDRESS,
                .scl_speed_hz = scl_speed_hz,
            };

            ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &this->dev_handle));

            uint8_t comPins = HEIGHT <= 32 ? 0x02 : 0x12;
            uint8_t mux_ratio = HEIGHT - 1;

            uint8_t contrast = 0x8F;

            if ((WIDTH == 128) && (HEIGHT == 32))
            {
                contrast = 0x8F;
            }
            else if ((WIDTH == 128) && (HEIGHT == 64))
            {
                contrast = EXTERNALVCC ? 0x9F : 0xCF;
            }
            else if ((WIDTH == 96) && (HEIGHT == 16))
            {
                contrast = EXTERNALVCC ? 0x10 : 0xAF;
            }
            else
            {
                // Other screen varieties -- TBD
            }

            uint8_t init_data[] = {
                CMD::WRITE_CMD_STREAM,
                CMD::DISPLAYOFF,
                CMD::SETMULTIPLEX, mux_ratio,
                CMD::SETDISPLAYOFFSET, 0,
                (uint8_t)(CMD::SETSTARTLINE | startline),
                FLIP ? CMD::SEGREMAP__0 : CMD::SEGREMAP__1,
                CMD::COMSCAN__DEC,
                CMD::SETDISPLAYCLOCKDIV, 0x80, //--set display clock divide ratio/oscillator frequency
                CMD::SETCOMPINS, comPins,
                CMD::SETCONTRAST, contrast,
                CMD::DISPLAYALLON_RESUME,
                CMD::SETVCOMDETECT, 0x40,
                CMD::MEMORYMODE, CMD::MEMORYMODE_PARAM_HORIZONTAL, // Page based adressing; default value
                0x00, 0x10,                                        // reset column to zero
                CMD::CHARGEPUMP, EXTERNALVCC ? CMD::CHARGEPUMP_PARAM_DISABLE : CMD::CHARGEPUMP_PARAM_ENABLE,
                CMD::DEACTIVATE_SCROLL,
                CMD::NORMALDISPLAY, // instead of invert!
                CMD::DISPLAYON};
            esp_err_t res;
            res = i2c_master_transmit(dev_handle, init_data, sizeof(init_data), 0);
            if (res == ESP_OK)
            {
                ESP_LOGI(TAG, "OLED configured successfully");
            }
            else
            {
                ESP_LOGE(TAG, "Could not write to device", );
            }
            clear_screen();

            return ESP_OK;
        }

        void fill_rectangle(uint8_t chXpos1, uint8_t chYpos1, uint8_t chXpos2, uint8_t chYpos2)
        {
            uint8_t chXpos, chYpos;

            for (chXpos = chXpos1; chXpos <= chXpos2; chXpos++)
            {
                for (chYpos = chYpos1; chYpos <= chYpos2; chYpos++)
                {
                    fillPoint(chXpos, chYpos);
                }
            }
        }

        void fillPoint(uint8_t x, uint8_t y)
        {
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
                return;
            size_t index = point2buffer_index(x, y);
            this->s_chDisplayBuffer[index] |= 1 << y & 0b111;
        }

        M(const FontDesc* const font, uint8_t startline = 0) : startline(startline & 0b00111111), font(font)
        {
            this->s_chDisplayBuffer = new uint8_t[128 * 8];
            memset(this->s_chDisplayBuffer, 0, 128 * 8);
            this->write_buf = new uint8_t[128 + 5];
        }

    private:
        i2c_master_dev_handle_t dev_handle{nullptr};

        uint8_t *s_chDisplayBuffer{nullptr};
        uint8_t *write_buf{nullptr};
        uint8_t startline;
        const FontDesc* const font;
        std::vector<GlyphHelper> glyphs;

        inline size_t point2buffer_index(uint8_t x, uint8_t y)
        {
            return (y >> 3) * WIDTH + x;
        }

        void _swap(int &x, int &y)
        {
            int z = x;
            x = y;
            y = z;
        }

        esp_err_t i2c_write(const uint8_t cmd_or_data, const uint8_t *const data, const uint16_t data_len)
        {
            uint8_t buf[data_len + 1];
            buf[0] = cmd_or_data;
            for (int i = 0; i < data_len; i++)
            {
                buf[i + 1] = data[i];
            }

            return i2c_master_transmit(dev_handle, buf, data_len + 1, -1);
        }

        esp_err_t ssd1306_write_data(const uint8_t *const data, const uint16_t data_len)
        {
            return i2c_write(CMD::WRITE_DAT, data, data_len);
        }

        esp_err_t ssd1306_write_cmd(const uint8_t *const data, const uint16_t data_len)
        {
            return i2c_write(CMD::WRITE_CMD, data, data_len);
        }

        esp_err_t ssd1306_write_cmd_byte(const uint8_t cmd)
        {
            return ssd1306_write_cmd(&cmd, 1);
        }
    };
}