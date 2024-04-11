#pragma once
#include "../lvgl.h"
#include <cstring>
#include "./interfaces.hh"
#include <bit>
#define TAG "FONT"
#include <esp_log.h>

TODO implement hqx3
TODO implement UTF8
TODO implement: RGB565 als Klasse, inter immer system-byte-order 16bit uint
es gibt eine Funktion toSPI_ST7789, in der das richtig Format erzeugt wird
es gibt eine Funktion alphaBlend

namespace SPILCD16
{
    constexpr const uint8_t opa4_table[16] = {0, 17, 34, 51,
                                              68, 85, 102, 119,
                                              136, 153, 170, 187,
                                              204, 221, 238, 255};
    class TextRenderer : public SPILCD16::IAsyncRenderer
    {
    private:
        const lv_font_t *font;
        Point2D pos;
        Color565 colors[16];
        const char *text;
        char *c;
        const lv_font_fmt_txt_glyph_dsc_t *glyph_dsc{nullptr};

    public:
        TextRenderer(const lv_font_t *font, Point2D start, Color565 foreground, Color565 background, char *text) : font(font), pos(start), text(text), c(text) {
            for(int i=0;i<16;i++){
                colors[i]=background.blendWith(foreground, opa4_table[i])
            }
        }

        bool GetNextOverallLimits(Point2D &start, Point2D &end_excl) override
        {
            if (c == 0 || *c == '\0')
            {
                free(c);
                c = nullptr;
                return false;
            }
            uint16_t codepoint = *c; // TODO: This is only ascii - we need UTF8
            uint32_t glyphIndex;
            if (!GetGlyphIndex(codepoint, glyphIndex))
            {
                return false;
            }
            uint16_t nextCodepoint=*(c+1);
            uint32_t nextGlyphIndex;
            GetGlyphIndex(nextCodepoint, nextGlyphIndex);
            
            glyph_dsc = font->dsc->glyph_dsc + glyphIndex;

            int8_t kvalue = GetKerningValue(glyphIndex, nextGlyphIndex);
            int32_t kv = ((int32_t)((int32_t)kvalue * font->dsc->kern_scale) >> 4); //kernscale==16, dann wieder durch 16 teilen, also keine Änderung


            start.x = pos.x + glyph_dsc->ofs_x;
            end_excl.x = start.x + glyph_dsc->box_w;
            end_excl.y = pos.y + glyph_dsc->ofs_y;
            start.y = end_excl.y - glyph_dsc->box_h;
            ESP_LOGI(TAG, "Write '%c'@%ld from (%d/%d) to (%d/%d)", *c, glyphIndex, start.x, start.y, end_excl.x, end_excl.y);
            c++;
            uint32_t adv_w = glyph_dsc->adv_w;
            adv_w += kv;
            adv_w  = (adv_w + (1 << 3)) >> 4;
            pos.x += (adv_w / 16);
            return true;
        }

        bool GetGlyphIndex(uint16_t codepoint, uint32_t &glyphIndex)
        {
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
                    glyphIndex = map.glyph_id_start + codepoint - map.range_start;
                    return true;
                case LV_FONT_FMT_TXT_CMAP_SPARSE_TINY:
                    for (int offset = 0; offset < map.list_length; offset++)
                    {
                        if (map.unicode_list[offset] == codepoint)
                        {
                            glyphIndex = map.glyph_id_start + offset;
                            return true;
                        }
                    }
                    break;
                default:
                    break;
                }
                break;
            }
            glyphIndex = 0;
            return false;
        }

        // opacity 0 bedeutet durchsichtig, opacity 255 bedeutet 100% deckung
        Color565 opacity(Color565 top, Color565 bottom, uint8_t opacity_of_top_color_0_255)
        {
            float alpha = ((float)opacity_of_top_color_0_255) / ((float)255);
            uint8_t red = (RGB565::R(top) * alpha + RGB565::R(bottom) * (1 - alpha));
            uint8_t green = (RGB565::G(top) * alpha + RGB565::G(bottom) * (1 - alpha));
            uint8_t blue = (RGB565::B(top) * alpha + RGB565::B(bottom) * (1 - alpha));
            // uint8_t green = (G(top)*opacity_of_top_color_0_255+G(bottom)*(255-opacity_of_top_color_0_255))/255;
            // uint8_t blue = (B(top)*opacity_of_top_color_0_255+B(bottom)*(255-opacity_of_top_color_0_255))/255;
            Color565 c = RGB565::RGBto565(red, green, blue);
            ESP_LOGI(TAG, "alpha:%f, rgb888:(%d/%d/%d), rgb565:%04X", alpha, red, green, blue, c);
            return c;
        }

        int8_t GetKerningValue(uint32_t gid_left, uint32_t gid_right)
        {
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
            return value;
        }

        void Render(uint32_t startPixel, uint16_t *buffer, size_t len) override
        {
            size_t expected = glyph_dsc->box_h * glyph_dsc->box_w;
            if (expected != len)
            {
                ESP_LOGE(TAG, "Expected len=%u, buffer len=%u", expected, len);
            }
            const uint8_t *bitmap = font->dsc->glyph_bitmap;
            uint32_t bo = glyph_dsc->bitmap_index;
            uint8_t bits = 0, bit = 0;
            size_t i = 0;
            if (font->dsc->bpp == 4)
            {
                // immer zwei pixel in einem rutsch
                while (i < len)
                {
                    bits = bitmap[bo++];
                    buffer[i++] = colors[(bits & 0xF0) >> 4];
                    if (i < len)
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
}
#undef TAG