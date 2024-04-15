#pragma once
#include <cstdint>
#include <lvgl/lvgl.h>
namespace SPILCD16
{
    constexpr const uint8_t opa4_table[16] = {0, 17, 34, 51,
                                              68, 85, 102, 119,
                                              136, 153, 170, 187,
                                              204, 221, 238, 255};

    uint32_t GetGlyphIndex(const lv_font_t *font, uint32_t codepoint)
    {
        if (!codepoint)
            return 0; // fail fast
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

    int32_t GetKerningValue(const lv_font_t *font, uint32_t gid_left, uint32_t gid_right)
    {
        assert(gid_left);
        if (!gid_right)
            return 0;
        const lv_font_fmt_txt_dsc_t *fdsc = font->dsc;

        int8_t value = 0;

        if (fdsc->kern_classes == 0)
        {
            // ESP_LOGW(TAG, "fdsc->kern_classes == 0 not supported");
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
        return (value * font->dsc->kern_scale) >> 4; // kernscale==16, dann wieder durch 16 teilen, also keine Änderung
    }

    uint32_t getCodepointAndAdvancePointer(char **c)
    {
        //if((**c)==0) return 0; not necessary; is first case!
        uint32_t codepoint{0};
        if (((**c) & 0b10000000) == 0)
        { // 1byte
            codepoint = **c;
            (*c) += 1;
        }
        else if (((**c) & 0b11100000) == 0b11000000)
        { // 2byte
            codepoint = (**c) & 0b00011111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
        }
        else if (((**c) & 0b11110000) == 0b11100000)
        { // 3byte
            codepoint = (**c) & 0b00001111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
        }
        else if (((**c) & 0b11111000) == 0b11110000)
        { // 4byte
            codepoint = (**c) & 0b00000111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
        }
        // ESP_LOGI(TAG, "Found codepoint %lu", codepoint);
        return codepoint;
    }
}