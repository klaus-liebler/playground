#pragma once
#include <cstdint>
#include <initializer_list>
#include <esp_log.h>
#define TAG "FONT"
namespace lcd_common
{
    class GlyphDesc
    {
    public:
        const uint32_t bitmap_index;
        const uint16_t adv_w;
        const uint8_t box_w;
        const uint8_t box_h;
        const uint8_t ofs_x;
        const uint8_t ofs_y;
        const uint16_t kerningClassLeft;
        const uint16_t kerningClassRight;
        const uint8_t bitmapFormat;
        constexpr GlyphDesc(uint32_t bitmap_index, uint16_t adv_w, uint8_t box_w, uint8_t box_h, uint8_t ofs_x, uint8_t ofs_x) : 
            bitmap_index(bitmap_index),
            adv_w(adv_w),
            box_w(box_w),
            box_h(box_h),
            ofs_x(ofs_x),
            ofs_y(ofs_y),
            kerningClassLeft(kerningClassLeft),
            kerningClassRight(kerningClassRight),
            bitmapFormat(bitmapFormat)
        {
        }
    };

    struct GlyphHelper{
        const GlyphDesc* glyph_dsc;
        int16_t startX;
    };


    class CharacterMap0Tiny
    {
    public:
        const uint32_t firstCodepoint;
        const uint32_t firstGlyph;
        const uint32_t len;
        bool GetGlyphIndex(uint32_t codepoint, uint32_t &glyphIndex) const
        {
            if ((codepoint < firstCodepoint) || (firstCodepoint + len <= codepoint))
            {
                return false;
            }
            glyphIndex = firstGlyph + (codepoint - firstCodepoint);
            return true;
        }
        constexpr CharacterMap0Tiny(uint32_t firstCodepoint, uint32_t firstGlyph, uint32_t len) : firstCodepoint(firstCodepoint),
                                                                                                  firstGlyph(firstGlyph),
                                                                                                  len(len)
        {
        }
    };

    class FontDesc
    {
    public:
        const uint16_t unitsPerEm;
        const uint8_t leftKerningClassCnt;
        const uint8_t rightKerningClassCnt;
        const std::initializer_list<int16_t> *kern_class_values;
        const uint32_t cmapsLen;
        const std::initializer_list<CharacterMap0Tiny> *cmaps;
        const std::initializer_list<GlyphDesc> *glyph_desc;
        const std::initializer_list<uint8_t> *glyph_bitmap;
        const int8_t line_height;
        const int8_t base_line;

        uint32_t GetGlyphIndex(uint32_t codepoint) const
        {
            if (!codepoint)
                return 0; // fail fast
            uint32_t glyphIndex;
            for (int mapIndex = 0; mapIndex < cmapsLen; mapIndex++)
            {
                auto map = cmaps->begin()[mapIndex];
                if (map.GetGlyphIndex(codepoint, glyphIndex))
                {
                    return glyphIndex;
                }
            }
            return GetGlyphIndex(' '); // if glyph is not found, return the glyph of "space"
        }

        int16_t GetKerningValue(GlyphDesc* left, GlyphDesc* right) const
        {
            if (!right || right->kerningClassRight=0)
                return 0;
           if (!left || left->kerningClassLeft=0)
                return 0;
            uint8_t left_class = left->kerningClassLeft;
            uint8_t right_class = right->kerningClassRight;

            /*If class = 0, kerning not exist for that glyph
             *else got the value form `class_pair_values` 2D array*/
            return kern_class_values->begin()[(left_class - 1) * rightKerningClassCnt + (right_class - 1)];
        }

        constexpr FontDesc(
            const uint16_t unitsPerEm,
            const uint8_t leftKerningClassCnt,
            const uint8_t rightKerningClassCnt,
            const std::initializer_list<int16_t> *kern_class_values,
            const uint32_t cmapsLen,
            const std::initializer_list<CharacterMap0Tiny> *cmaps,
            const std::initializer_list<GlyphDesc> *glyph_desc,
            const std::initializer_list<uint8_t> *glyph_bitmap,
            const int8_t line_height,
            const int8_t base_line) : unitsPerEm(unitsPerEm),
                                      leftKerningClassCnt(leftKerningClassCnt),
                                      rightKerningClassCnt(rightKerningClassCnt),
                                      kern_class_values(kern_class_values),
                                      cmapsLen(cmapsLen),
                                      cmaps(cmaps),
                                      glyph_desc(glyph_desc),
                                      glyph_bitmap(glyph_bitmap),
                                      line_height(line_height),
                                      base_line(base_line)
        {
        }
    };

    class FontDesc_1bpp_8rows: public FontDesc{
        size_t WriteGlyphLineToBuffer16(const GlyphHelper* gh, uint16_t startline, uint16_t l, uint16_t p, uint16_t *buffer, uint16_t* colors, uint16_t suppressedPixelIfBackground = 0)
        {
            const uint8_t *bitmap = this->glyph_bitmap->begin();
            uint32_t bo = gh->glyph_dsc->bitmap_index;
            uint16_t lineInBitmap=startline+l-gh->glyph_dsc->ofs_y;
            uint16_t lineOf8= lineInBitmap>>3;
            uint16_t lineIn8 = lineInBitmap&7;

            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = lineOf8 * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + indexInBitmap];
                uint8_t bit = bits&(1<<lineIn8);
                
                if (x >= suppressedPixelIfBackground || bit)
                { // ausgabe nur dann, wenn wir sowieso außerhalb der suppressed sind oder die Farbe Nicht-Hintergrund ist
                    buffer[l * LINE_WIDTH_PIXELS + p] = bit?colors[15]:colors[0];
                }
                p++;
            }
            return p;
        }
        
        
        size_t WriteGlyphLineToBuffer1(const GlyphHelper* gh, bool invert, uint16_t ramline, uint8_t *buffer)
        {
            const uint8_t *bitmap= this->glyph_bitmap->begin();
            uint32_t bo = gh->glyph_dsc->bitmap_index;
            ESP_LOGD(TAG, "bitmap_base_address =0x%08X, glyph_bitmap_offset=%lu", (unsigned int)bitmap, bo);
            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = ramline * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + indexInBitmap];  
                ESP_LOGD(TAG, "indexInBitmap=%i, bits=0x%02X", indexInBitmap, bits);
                if(!invert){
                    buffer[startX+x]|=bits;
                }else{
                    buffer[startX+x]&=~bits;
                }
            }
            return glyph_dsc->box_w;
        }
    };

    constexpr const uint8_t opa4_table[16] = {0, 17, 34, 51,
                                              68, 85, 102, 119,
                                              136, 153, 170, 187,
                                              204, 221, 238, 255};


    class FontDesc_4bpp_2columns: public FontDesc{
        size_t WriteGlyphLineToBuffer(const GlyphHelper* gh, uint16_t startline, uint16_t l, uint16_t p, uint16_t *buffer, uint16_t* colors, uint16_t suppressedPixelIfBackground = 0)
        {

            const uint8_t *bitmap = this->glyph_bitmap->begin();
            uint32_t bo = gh->glyph_dsc->bitmap_index;
            uint16_t lineInBitmap=startline+l-gh->glyph_dsc->ofs_y;

            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = lineInBitmap * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + (indexInBitmap >> 1)];                                      // weil 4bpp, muss der Index halbiert werden und der grundsätzliche offset des glyphs berücksichtigt werden
                uint8_t color_index = (indexInBitmap % 2 == 0) ? ((bits & 0xF0) >> 4) : (bits & 0x0F); // geade indizes stehen in den MSB, ungerade in den vier LSB
                if (x >= suppressedPixelIfBackground || color_index != 0)
                { // ausgabe nur dann, wenn wir sowieso außerhalb der suppressed sind oder die Farbe Nicht-Hintergrund ist
                    buffer[l * LINE_WIDTH_PIXELS + p] = colors[color_index];
                }
                p++;
            }
            return p;
        }
    };


}

namespace arial_and_symbols_16px1bpp
{
    extern const lcd_common::FontDesc font;
}
#undef TAG