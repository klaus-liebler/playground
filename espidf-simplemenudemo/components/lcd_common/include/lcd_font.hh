#pragma once
#include <cstdint>
#include <initializer_list>
namespace lcd_common
{
    class GlyphDesc
    {
    public:
        const uint32_t bitmap_index;
        const uint16_t adv_w;
        const uint8_t box_w;
        const uint8_t ofs_x;
        constexpr GlyphDesc(uint32_t bitmap_index, uint16_t adv_w, uint8_t box_w, uint8_t ofs_x) : bitmap_index(bitmap_index),
                                                                                                   adv_w(adv_w),
                                                                                                   box_w(box_w),
                                                                                                   ofs_x(ofs_x)
        {
        }
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
        const uint32_t maxGlyphIndexWithKerningInfo;
        const uint8_t leftKerningClassCnt;
        const uint8_t rightKerningClassCnt;
        const std::initializer_list<uint8_t> *kern_left_class_mapping;
        const std::initializer_list<uint8_t> *kern_right_class_mapping;
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

        int16_t GetKerningValue(uint32_t gid_left, uint32_t gid_right) const
        {

            if (!gid_right)
                return 0;
            if (gid_left > maxGlyphIndexWithKerningInfo || gid_right > maxGlyphIndexWithKerningInfo)
            {
                return 0;
            }

            uint8_t left_class = kern_left_class_mapping->begin()[gid_left];
            uint8_t right_class = kern_right_class_mapping->begin()[gid_right];

            /*If class = 0, kerning not exist for that glyph
             *else got the value form `class_pair_values` 2D array*/
            if (left_class == 0 || right_class == 0)
            {
                return 0;
            }

            return kern_class_values->begin()[(left_class - 1) * rightKerningClassCnt + (right_class - 1)];
        }

        constexpr FontDesc(
            const uint16_t unitsPerEm,
            const uint32_t maxGlyphIndexWithKerningInfo,
            const uint8_t leftKerningClassCnt,
            const uint8_t rightKerningClassCnt,
            const std::initializer_list<uint8_t> *kern_left_class_mapping,
            const std::initializer_list<uint8_t> *kern_right_class_mapping,
            const std::initializer_list<int16_t> *kern_class_values,
            const uint32_t cmapsLen,
            const std::initializer_list<CharacterMap0Tiny> *cmaps,
            const std::initializer_list<GlyphDesc> *glyph_desc,
            const std::initializer_list<uint8_t> *glyph_bitmap,
            const int8_t line_height,
            const int8_t base_line) : unitsPerEm(unitsPerEm),
                                      maxGlyphIndexWithKerningInfo(maxGlyphIndexWithKerningInfo),
                                      leftKerningClassCnt(leftKerningClassCnt),
                                      rightKerningClassCnt(rightKerningClassCnt),
                                      kern_left_class_mapping(kern_left_class_mapping),
                                      kern_right_class_mapping(kern_right_class_mapping),
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
}

namespace arial_and_symbols_16px1bpp
{
    extern const lcd_common::FontDesc font;
}
