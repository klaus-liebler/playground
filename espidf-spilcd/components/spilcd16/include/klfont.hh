#pragma once
class GlyphDesc
{
public:
    const uint32_t bitmap_index;
    const uint16_t adv_w;
    const uint8_t box_w;
    const uint8_t ofs_x;
    constexpr GlyphDesc(uint32_t bitmap_index, uint16_t adv_w, uint8_t box_w = 0, uint8_t ofs_x) : bitmap_index(bitmap_index),
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
    constexpr CharacterMap0Tiny(uint32_t firstCodepoint, uint32_t firstGlyph, uint32_t len) : firstCodepoint(firstCodepoint),
                                                                                              firstGlyph(firstGlyph),
                                                                                              len(len)
    {
    }
};
