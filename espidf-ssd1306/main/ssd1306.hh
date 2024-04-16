#pragma once
#include <algorithm>
#include <driver/i2c_master.h>
#include <driver/gpio.h>
#include "string.h" // for memset
#define TAG "LCD"
#include <esp_log.h>
#include <ssd1306_fonts.hh>
#include <ssd1306_cmd.hh>

#include <stdio.h>



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

    /**
     * Allways positive; for array indices!
     */
    inline size_t modulo(const int i, const int n)
    {
        return (i % n + n) % n;
    }

    template <uint8_t WIDTH = 128, uint8_t HEIGHT = 32, uint8_t I2C_ADDRESS = 0x3C, bool EXTERNALVCC = false, bool FLIP = false>
    class M : public FullLineWriter
    {

    public:
        uint8_t GetShownLines() override
        {
            return HEIGHT / (font->h);
        }

        uint8_t GetAvailableLines() override
        {
            return 64 / (font->h);
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

            SetStartline((float)(this->font->h * start_textline_nominator) / (float)start_textline_denominator);
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
            int step = ((this->font->h) / 4) * (textlines < 0 ? -1 : +1);
            int count = 4 * abs(textlines);
            ESP_LOGI(TAG, "Scoll h=%d count=%d, step=%d, old startline=%d", this->font->h, count, step, startline);
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

        const uint8_t *GetCharImage(char char_code, uint8_t ramline)
        {
            char_code -= (font->ascii_offset);
            size_t bytes_per_char = font->h * font->w / 8;
            return (font->first_character_data + bytes_per_char * char_code) + ramline * bytes_per_char / 2;
        }

        const uint8_t *GetSymbolImage(char char_code, uint8_t ramline)
        {
            char_code--;
            return font::icons16x16 + (32 * char_code) + 16 * ramline;
        }
    /*
        size_t printchar_ttf(int line, SFT *sft, unsigned long cp)
        {
            SFT_Glyph gid; //  unsigned long gid;
            if (sft_lookup(sft, cp, &gid) < 0){
                ESP_LOGE(TAG, "Missing codepoint %lu", cp);
                return 0;
            }
            ESP_LOGI(TAG, "lookup successful");
            SFT_GMetrics mtx;
            if (sft_gmetrics(sft, gid, &mtx) < 0){
                ESP_LOGE(TAG, "Bad glyph metrics codepoint %lu", cp);
                return 0;
            }
            
            SFT_Image img = {};
            img.width = (mtx.minWidth + 3) & ~3;
            img.height = mtx.minHeight;
            
            ESP_LOGI(TAG, "sft_gmetrics successful imgWidth=%d imgHeight=%d", img.width, img.height);
            char pixels[img.width * img.height];
            img.pixels = pixels;

            if (sft_render(sft, gid, img) < 0){
                ESP_LOGE(TAG, "Not renderable codepoint %lu", cp);
                return 0;
            }
            ESP_LOGI(TAG, "Successfully rendered char '%c' with w=%d and h=%d  ", (char)cp, img.width, img.height);
            for(int y=0;y<img.height;y++){
                for(int x=0;x<img.width;x++){
                    printf("%c",pixels[y*img.width+x]);
                }
                printf("\n");
            }

            return 0;
        }
*/
        size_t printfl(int line, bool invert, const char *format, ...)
        {
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
            uint8_t *out_buf = new uint8_t[WIDTH + 1];
            for (int ramline = 0; ramline < (font->h) / 8; ramline++)
            {
                setWindowFullPage(ramline + line * (font->h) / 8);
                out_buf[0] = CMD::WRITE_DAT;
                uint8_t seg = 0;
                uint8_t char_index = 0;
                bool horizontalTabDetected{false};
                out_buf[1 + seg] = invert ? 0xFF : 0x00;
                seg++;
                while (char_index < chars_len)
                {
                    char char_code = chars[char_index];
                    if (char_code == 9)
                    { // Horizontal tab -->write from right
                        horizontalTabDetected = true;
                        break;
                    }
                    const uint8_t *char_image{nullptr};
                    size_t char_width = 8;
                    if (char_code == 0x1B) // Escape character -->goto symbol font
                    {
                        char_code = chars[++char_index];
                        // char_image = font::icons8x8_data[(uint8_t)char_code];
                        char_image = GetSymbolImage(char_code, ramline);
                        char_width = 16;
                    }
                    else
                    {
                        char_image = GetCharImage(char_code, ramline);
                    }

                    for (int i = 0; i < char_width; i++)
                    {
                        out_buf[1 + seg] = invert ? ~char_image[i] : char_image[i];
                        seg++;
                    }
                    char_index++;
                }
                while (seg < WIDTH)
                {
                    out_buf[1 + seg] = invert ? 0xFF : 0x00;
                    seg++;
                }

                if (horizontalTabDetected)
                {
                    char_index = chars_len - 1;
                    seg = WIDTH - 2;
                    while (char_index > 0)
                    {
                        char char_code = chars[char_index];
                        if (char_code == 9)
                        { // Horizontal tab -->write from right
                            break;
                        }
                        const uint8_t *char_image{nullptr};
                        size_t char_width = 8;
                        if (chars[char_index - 1] == 0x1B)
                        {
                            char_image = GetSymbolImage(char_code, ramline);
                            char_index -= 2;
                            char_width = 16;
                        }
                        else
                        {
                            char_image = GetCharImage(char_code, ramline);
                            char_index -= 1;
                        }
                        for (int i = char_width - 1; i >= 0; i--)
                        {
                            out_buf[1 + seg] = invert ? ~char_image[i] : char_image[i];
                            seg--;
                        }
                    }
                }

                ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, out_buf, WIDTH + 1, I2C_TICKS_TO_WAIT));
            }
            free(chars);
            delete[] out_buf;
            return chars_len;
        }

        size_t lcdprintf(int line, uint8_t segment0_127, bool invert, const char *format, ...)
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
                update_buf(line, segment0_127, font::font8x8_data[(uint8_t)buffer[i]], 8, invert);
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
            ESP_LOGI(TAG, "Found SSD1306");

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

        M(const font::FixedSizeFont *font, uint8_t startline = 0) : startline(startline & 0b00111111), font(font)
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
        const font::FixedSizeFont *font;

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