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
                colors[i]=background.blendWith(foreground, opa)
            }
        }

        bool GetNextOverallLimits(Point2D &start, Point2D &end_excl) override
        {
            if (c==0 || *c == '\0'){
                free(c);
                c=nullptr;
                return false;
            }
            uint16_t codepoint = *c; // TODO: This is only ascii - we need UTF8
            int32_t glyphIndex;
            if (!GetGlyphIndex(codepoint, glyphIndex))
            {
                return false;
            }
            glyph_dsc = font->dsc->glyph_dsc + glyphIndex;
            start.x = pos.x + glyph_dsc->ofs_x;
            end_excl.x = start.x + glyph_dsc->box_w;
            end_excl.y = pos.y + glyph_dsc->ofs_y;
            start.y = end_excl.y - glyph_dsc->box_h;
            ESP_LOGI(TAG, "Write '%c'@%ld from (%d/%d) to (%d/%d)", *c, glyphIndex, start.x, start.y, end_excl.x, end_excl.y);
            c++;
            pos.x+=(glyph_dsc->adv_w/16);
            return true;
        }

        bool GetGlyphIndex(uint16_t codepoint, int32_t &glyphIndex)
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
            glyphIndex = -1;
            return false;
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
            if(font->dsc->bpp==4){
                //immer zwei pixel in einem rutsch
                
                while (i<len)
                {
                    bits = bitmap[bo++];
                    buffer[i++] = bits & 0xF0 ? foreground : background;
                    if(i<len)buffer[i++] = bits & 0x0F ? foreground : background;
                }
                
            }
            
            for (size_t yy = 0; yy < glyph_dsc->box_h; yy++)
            {
                for (size_t xx = 0; xx < glyph_dsc->box_w; xx++)
                {
                    if (!(bit++ & 7))
                    {
                        bits = bitmap[bo++];
                    }
                    buffer[i] = bits & 0x80 ? foreground : background;
                    i++;
                    bits <<= 1;
                }
            }
        }
    };
}
#undef TAG