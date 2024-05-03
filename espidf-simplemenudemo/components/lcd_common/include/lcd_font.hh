#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <initializer_list>
#include <esp_log.h>
#include "unicode_utils.hh"
#define TAG "FONT"
namespace lcd_common
{
    enum class BitmapFormat{
        UNDEFINED,
        ONE_BPP_EIGHT_IN_A_COLUMN,//best for SSD1306 etc
        FOUR_BPP_ROW_BY_ROW,//best for ST7789 etc
    };
    
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
        const BitmapFormat bitmapFormat;
        constexpr GlyphDesc(uint32_t bitmap_index, uint16_t adv_w, uint8_t box_w, uint8_t box_h, uint8_t ofs_x, uint8_t ofs_y, uint16_t kerningClassLeft, uint16_t kerningClassRight, BitmapFormat bitmapFormat) :
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

        const GlyphDesc* GetGlyphDesc(uint32_t codepoint) const{
            auto ptr =  &glyph_desc->begin()[GetGlyphIndex(codepoint)];
            if(ptr>glyph_desc->end()) return nullptr;
            return ptr;
        }

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

        int16_t GetKerningValue(const GlyphDesc *left, const GlyphDesc *right) const
        {
            if (!right || right->kerningClassRight == 0)
                return 0;
            if (!left || left->kerningClassLeft == 0)
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

    constexpr const uint8_t opa4_table[16] = {0, 17, 34, 51,
                                              68, 85, 102, 119,
                                              136, 153, 170, 187,
                                              204, 221, 238, 255};
class GlyphHelper
    {
        public:
        const GlyphDesc *glyph_dsc;
        int16_t startX;//position of bitmap

        static void PopulateGlyphs(const FontDesc* const font, char *chars, std::vector<GlyphHelper>& glyphs, uint16_t PADDING_LEFT, uint16_t LINE_WIDTH_PIXELS, uint16_t PADDING_RIGHT)
        {
            uint32_t currentCodepoint = unicode_utils::getCodepointAndAdvancePointer(&chars);
            auto currentGlyph=font->GetGlyphDesc(currentCodepoint);
            uint32_t nextCodepoint{0};
            const GlyphDesc* nextGlyph{nullptr};
            //two tabs are supported. First tab is centered and center-aligned
            //second tab is right and right-aligned
            size_t glyphBeforeTabulator[2]={UINT32_MAX, UINT32_MAX};
            
            uint16_t posX = PADDING_LEFT-currentGlyph->ofs_x;
            uint16_t endX{0};
            uint8_t tabIndex=0;
            while (currentCodepoint)
            {
                
                nextCodepoint = unicode_utils::getCodepointAndAdvancePointer(&chars);
                int32_t kv;
                if(nextCodepoint=='\t' && tabIndex<2){
                    glyphBeforeTabulator[tabIndex++]=glyphs.size();
                    nextCodepoint = unicode_utils::getCodepointAndAdvancePointer(&chars);
                    if(nextCodepoint=='\t' && tabIndex<2){
                        glyphBeforeTabulator[tabIndex++]=glyphs.size();
                        nextCodepoint = unicode_utils::getCodepointAndAdvancePointer(&chars);
                        ESP_LOGD(TAG, "Two Tabs detected! pos=%d, codePointAfter=%lu", glyphs.size(), nextCodepoint);
                    }
                    else{
                        ESP_LOGD(TAG, "One Tab detected! pos=%d, codePointAfter=%lu", glyphs.size(), nextCodepoint);
                    }
                    nextGlyph = font->GetGlyphDesc(nextCodepoint);
                    kv=0;
                    
                } else{
                    nextGlyph = font->GetGlyphDesc(nextCodepoint);
                    kv = font->GetKerningValue(currentGlyph, nextGlyph);
                }

                GlyphHelper gh = {};
                gh.glyph_dsc = currentGlyph;
                gh.startX = posX + gh.glyph_dsc->ofs_x;
               
                endX = gh.startX + gh.glyph_dsc->box_w;
                if (endX >= LINE_WIDTH_PIXELS)
                {
                    ESP_LOGW(TAG, "NOT push GlyphIndex, posX=%d endX=%d, startX=%d", posX, endX, gh.startX);
                    break; // Damit ist sicher gestellt, dass man bei der Ausgabe keinerlei überprüfung machen muss, ob irgendwelche Grenzen überschritten werden -->einfach glyphs zeichnen und gut!
                }
                if(font->unitsPerEm==0){
                    posX += (gh.glyph_dsc->adv_w + kv);
                }else{
                    posX += ((gh.glyph_dsc->adv_w + kv) + (1 << 6)) >> 7;
                }
                glyphs.push_back(gh);
                currentCodepoint=nextCodepoint;
                currentGlyph=nextGlyph;
            }
            //Now, all glyphs are in the vector ... but still left aligned. Now, consider tabulators and move the glyphs back and forth
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
        
        size_t WriteGlyphLineToBuffer16bpp(const FontDesc* font, uint16_t line, uint16_t *buffer_at_correct_position, uint16_t *colors, uint16_t suppressedPixelIfBackground = 0){
            switch (glyph_dsc->bitmapFormat)
            {
            case BitmapFormat::ONE_BPP_EIGHT_IN_A_COLUMN:
                return WriteGlyphLine1bppToBuffer16bpp(font, line, buffer_at_correct_position, colors, suppressedPixelIfBackground);
            case BitmapFormat::FOUR_BPP_ROW_BY_ROW:
                return WriteGlyphLine4bppToBuffer16bpp(font, line, buffer_at_correct_position, colors, suppressedPixelIfBackground);
            default:
                ESP_LOGE(TAG, "BitmapFormat::FOUR_BPP_ROW_BY_ROW or other not yet supported for 1bpp displays");
                return 0;
            }
            
        }

        size_t WriteGlyphLineToBuffer1bpp(const FontDesc* font, bool invert, uint16_t ramline, uint8_t *buffer){
            switch (glyph_dsc->bitmapFormat)
            {
            case BitmapFormat::ONE_BPP_EIGHT_IN_A_COLUMN:
                return WriteGlyphLine1bppToBuffer1bpp(font, invert, ramline, buffer);
            default:
                ESP_LOGE(TAG, "BitmapFormat::FOUR_BPP_ROW_BY_ROW or other not yet supported for 1bpp displays");
                return 0;
            }
        }

        size_t WriteGlyphLine1bppToBuffer16bpp(const FontDesc* font, uint16_t line, uint16_t *buffer_at_correct_position, uint16_t *colors, uint16_t suppressedPixelIfBackground = 0)
        {
            int16_t lineInBitmap = line - this->glyph_dsc->ofs_y;
            if(lineInBitmap<0 || lineInBitmap>=this->glyph_dsc->box_h){
                //Wir sind außerhalb der Bitmap...und schreiben deshalb einfach nur "Hintergrundfarbe in den Buffer"
                for (int x = suppressedPixelIfBackground; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
                {
                    // ausgabe nur dann, wenn wir sowieso außerhalb der suppressed sind oder die Farbe Nicht-Hintergrund ist
                    buffer_at_correct_position[x] = colors[0];
                }
                return glyph_dsc->box_w;
            }
            const uint8_t *bitmap = font->glyph_bitmap->begin();
            uint32_t bo = this->glyph_dsc->bitmap_index;
            uint16_t lineOf8 = lineInBitmap >> 3;
            uint16_t lineIn8 = lineInBitmap & 7;

            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = lineOf8 * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + indexInBitmap];
                uint8_t bit = bits & (1 << lineIn8);

                if (x >= suppressedPixelIfBackground || bit)
                { // ausgabe nur dann, wenn wir sowieso außerhalb der suppressed sind oder die Farbe Nicht-Hintergrund ist
                    buffer_at_correct_position[x] = bit ? colors[15] : colors[0];
                }
                
            }
            return glyph_dsc->box_w;
        }

        size_t WriteGlyphLine4bppToBuffer16bpp(const FontDesc* font, uint16_t line, uint16_t *buffer_at_correct_position, uint16_t *colors, uint16_t suppressedPixelIfBackground = 0)
        {

            const uint8_t *bitmap = font->glyph_bitmap->begin();
            uint32_t bo = this->glyph_dsc->bitmap_index;
            int16_t lineInBitmap = line - this->glyph_dsc->ofs_y;
            if(lineInBitmap<0){
                return 0;
            }
            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = lineInBitmap * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + (indexInBitmap >> 1)];                                      // weil 4bpp, muss der Index halbiert werden und der grundsätzliche offset des glyphs berücksichtigt werden
                uint8_t color_index = (indexInBitmap % 2 == 0) ? ((bits & 0xF0) >> 4) : (bits & 0x0F); // geade indizes stehen in den MSB, ungerade in den vier LSB
                if (x >= suppressedPixelIfBackground || color_index != 0)
                { // ausgabe nur dann, wenn wir sowieso außerhalb der suppressed sind oder die Farbe Nicht-Hintergrund ist
                    buffer_at_correct_position[x] = colors[color_index];
                }
               
            }
            return glyph_dsc->box_w;
        }

        size_t WriteGlyphLine1bppToBuffer1bpp(const FontDesc* font, bool invert, uint16_t ramline, uint8_t *buffer)
        {
            const uint8_t *bitmap = font->glyph_bitmap->begin();
            uint32_t bo = glyph_dsc->bitmap_index;
            ESP_LOGD(TAG, "bitmap_base_address =0x%08X, glyph_bitmap_offset=%lu", (unsigned int)bitmap, bo);
            for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entlang
            {
                int indexInBitmap = ramline * glyph_dsc->box_w + x;
                uint8_t bits = bitmap[bo + indexInBitmap];
                ESP_LOGD(TAG, "indexInBitmap=%i, bits=0x%02X", indexInBitmap, bits);
                if (!invert)
                {
                    buffer[startX + x] |= bits;
                }
                else
                {
                    buffer[startX + x] &= ~bits;
                }
            }
            return glyph_dsc->box_w;
        }
    };

    

}

namespace arial_and_symbols_16px1bpp
{
    extern const lcd_common::FontDesc font;
}
#undef TAG