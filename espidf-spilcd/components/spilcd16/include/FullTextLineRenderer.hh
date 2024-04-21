#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <bit>
#include <esp_log.h>
#include "RGB565.hh"
#include "lvgl/lvgl.h"
#define TAG "FTLR"
#include "interfaces.hh"
#include "lvgl/lvgl.h"
#include "text_utils.hh"
using namespace display;
namespace SPILCD16
{
    template <uint8_t LINE_HEIGHT_PIXELS, uint8_t LINE_WIDTH_PIXELS, uint8_t PADDING_LEFT, uint8_t PADDING_RIGHT>
    class FullTextlineRenderer : public IAsyncRenderer
    {
    private:
        class GlyphHelper
        {
        public:
            const lv_font_fmt_txt_glyph_dsc_t *glyph_dsc;

            int16_t startY; // mit Bezug zum topleft. ab dort beginnt das Bitmap; Y-Achse geht nach unten entsprechend Bildschirm
            int16_t startX;  // mit bezug zum topleft, ab wo das bitmap startet
            bool BitmapDefinesLine(uint16_t l)
            {
                return !(l < startY || l >= startY + glyph_dsc->box_h);
            }

            /*
            p muss an der startX-Position des Bitmaps stehen
            suppressedPixelIfBackground: so viele Pixel werden im BUffer nicht überschreiben, wenn Sie nur Hintergrund sind (wichtig beim Kerning)
            */
            uint16_t WriteLineToBuffer(const lv_font_t *font, uint16_t startline, uint16_t l, uint16_t p, uint16_t *buffer, uint16_t* colors, uint16_t suppressedPixelIfBackground = 0)
            {

                const uint8_t *bitmap = font->dsc->glyph_bitmap;
                assert(startline+l >= startY);
                assert(p == startX);
                assert(font->dsc->bpp == 4);
                uint32_t bo = glyph_dsc->bitmap_index;
                uint16_t lineInBitmap=startline+l-startY;

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

        const lv_font_t *font;
        Point2D topfleft; // ist absolute Bildschirmkoordinate obere linke Ecke
        uint16_t colors[16];
        std::vector<GlyphHelper> glyphs;
        bool getNextOverallLimitsAlreadySent{false};
        

        void PopulateGlyphs(char *chars)
        {
            int baselineY = (LINE_HEIGHT_PIXELS + font->line_height-4) * 0.5; // font soll in der Mitte der LINE stehen
            ESP_LOGD(TAG, "Baseline=%d", baselineY);
            uint32_t currentCodepoint = getCodepointAndAdvancePointer(&chars);
            uint32_t currentGlyphIndex=GetGlyphIndex(font, currentCodepoint);
            uint32_t nextCodepoint{0};
            uint32_t nextGlyphIndex{0};
            //two tabs are supported. First tab is centered and center-aligned
            //second tab is right and right-aligned
            size_t glyphBeforeTabulator[2]={UINT32_MAX, UINT32_MAX};
            
            uint16_t posX = PADDING_LEFT-GetGlyphDesc(currentGlyphIndex)->ofs_x;
            uint16_t endX{0};
            uint8_t tabIndex=0;
            while (currentCodepoint)
            {
                
                nextCodepoint = getCodepointAndAdvancePointer(&chars);
                int32_t kv;
                if(nextCodepoint=='\t'){
                    glyphBeforeTabulator[(tabIndex%2)++]=glyphs.size();
                    nextCodepoint = getCodepointAndAdvancePointer(&chars);
                    nextGlyphIndex = GetGlyphIndex(font, nextCodepoint);
                    kv=0;
                    ESP_LOGI(TAG, "Tab detected! pos=%d, codePointAfter=%lu", glyphs.size(), nextCodepoint);
                } else{
                    nextGlyphIndex = GetGlyphIndex(font, nextCodepoint);
                    kv = GetKerningValue(font, currentGlyphIndex, nextGlyphIndex);
                }
                auto dsc=GetGlyphDesc(currentGlyphIndex);
                if(currentCodepoint>0xFF){
                    ESP_LOGD(TAG, "Special Codepoint detected %lu, glyphIndex=%lu, bitmapIndex=%i", currentCodepoint, currentGlyphIndex, dsc->bitmap_index);
                }

                GlyphHelper gh = {};
                gh.glyph_dsc = dsc;
                gh.startX = posX + gh.glyph_dsc->ofs_x;
                gh.startY = baselineY - gh.glyph_dsc->ofs_y - gh.glyph_dsc->box_h;
                endX = gh.startX + gh.glyph_dsc->box_w;
                if (endX >= LINE_WIDTH_PIXELS)
                {
                    ESP_LOGW(TAG, "NOT push GlyphIndex=%lu, posX=%d endX=%d, startX=%d, startY=%d", currentGlyphIndex, posX, endX, gh.startX, gh.startY);
                    break; // Damit ist sicher gestellt, dass man bei der Ausgabe keinerlei überprüfung machen muss, ob irgendwelche Grenzen überschritten werden -->einfach glyphs zeichnen und gut!
                }else{
                    ESP_LOGD(TAG, "push GlyphIndex=%lu, posX=%d nextIndex=%lu, kv=%lu, startX=%d, startY=%d", currentGlyphIndex, posX, nextGlyphIndex, kv, gh.startX, gh.startY);
                }
                posX += ((gh.glyph_dsc->adv_w + kv) + (1 << 3)) >> 4;
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
                first
            }
            
            if(glyphBeforeTabulator[0]!=UINT32_MAX && glyphBeforeTabulator[0]!=glyphBeforeTabulator[1]){
                GlyphHelper* g1 =&glyphs.at(glyphBeforeTabulator[0]+1);
                GlyphHelper* g2 =&glyphs.at(glyphBeforeTabulator[1]);
                
                uint16_t startOfBlock=g1->startX;
                uint16_t endOfBlock =g2->startX+g2->glyph_dsc->box_w;
                assert(endOfBlock>startOfBlock);
                uint16_t widthOfCenteredChars=endOfBlock-startOfBlock;
                uint16_t startPos = (LINE_HEIGHT_PIXELS/2)-(widthOfCenteredChars/2)
                offset=startPos-g1->startX;
                assert(offset>0);
                while(i>glyphBeforeTabulator[0]){
                    glyphs.at(i).startX+=offset;
                    ESP_LOGD(TAG, "moved Glyph at pos %d to posX=%d",  i, glyphs.at(i).startX);
                    i--;
                }

            }
            offset=
        }

    public:
        
        FullTextlineRenderer(const lv_font_t *font) : font(font)
        {
           
        }

        
        
        size_t printfl(uint8_t line, Color::Color565 foreground, Color::Color565 background, const char *format, ...)
        {
            va_list args_list;
            va_start(args_list, format);
            size_t ret = printfl(line, foreground, background, format, args_list);
            va_end(args_list);
            return ret;
        }

        size_t printfl(uint8_t line, Color::Color565 foreground, Color::Color565 background, const char *format, va_list args_list)
        {
            char *chars{nullptr};
            int chars_len = vasprintf(&chars, format, args_list);
            ESP_LOGI(TAG, "Printing '%s' to line %d", chars, line);
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, chars, chars_len, ESP_LOG_INFO);
            
            if (chars_len <= 0)
            {
                free(chars);
                return 0;
            }
            getNextOverallLimitsAlreadySent=false;
            
            for (int i = 0; i < 16; i++)
            {
                colors[i] = background.overlayWith(foreground, opa4_table[i]).toST7789_SPI_native();
            }
            
            this->topfleft =Point2D(0, line * LINE_HEIGHT_PIXELS);
            glyphs.clear();
            this->PopulateGlyphs(chars);
            free(chars);
            return glyphs.size();
        }

        bool GetNextOverallLimits(size_t bufferSize, Point2D &start, Point2D &end_excl) override
        {
            if (getNextOverallLimitsAlreadySent)
            {
                return false;
            }
            start.x = 0;
            start.y = topfleft.y;
            end_excl.x = LINE_WIDTH_PIXELS;
            end_excl.y = topfleft.y + LINE_HEIGHT_PIXELS;
            getNextOverallLimitsAlreadySent = true;
            return true;
        }

        const lv_font_fmt_txt_glyph_dsc_t *GetGlyphDesc(uint32_t glyphIndex)
        {
            return font->dsc->glyph_dsc + glyphIndex;
        }

        uint16_t WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(int ghIndex, uint16_t l, uint16_t p, uint16_t *buffer)
        {
            size_t end = LINE_WIDTH_PIXELS;
            if (ghIndex < glyphs.size())
            {
                GlyphHelper *g = &glyphs[ghIndex];
                end = g->startX;
            }
            while (p < end)
            {
                buffer[l * LINE_WIDTH_PIXELS + p] = colors[0];
                p++;
            }
            assert(p==end);
            return p;
        }

        void Render(uint16_t startline, uint16_t cnt, uint16_t *buffer) override
        {
            
            // es ist sicher gestellt, dass wir am Anfang der linie beginnen
            for (uint16_t l = 0; l < cnt; l++)//wir schreiben
            //das "l" meint hier die relative Zeile innerhalb des aktuellen buffersegments
            {
                ESP_LOGD(TAG, "Line %d+%d with %u glyps starts", startline, l, glyphs.size());
                int ghIndex = 0;
                uint16_t p = 0;
                while (ghIndex < glyphs.size())
                {
                    // Verarbeite eine Zeile eines glyph pro iteration
                    // Hole also den nächsten glyph
                    GlyphHelper *g = &glyphs[ghIndex];
                    if (!g->BitmapDefinesLine(startline+l))
                    {
                        
                        // Fall A: für diese Zeile exisitert keine Bitmap
                        // schreibe ausgehend vom aktuellen pixel (nicht vom eingetragenen startX, das gilt nur für Bitmap-lines) die Hintergrund-Farbe, und zwar bis zum Beginn des nächsten glyph bzw bis zum Ende der line
                        p = WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex + 1, l, p, buffer);
                        ESP_LOGD(TAG, "!glyph[%u]->BitmapDefinesLine(%d) -->write background till start of next glyph@%dpx", ghIndex, l, p);
                    }
                    else
                    {
                        // Fall B: für diese Zeile existieren Einträge in der Bitmap
                        // gehe an den Anfang des Bitmaps
                        if (p > g->startX)
                        {
                            // wenn wir durch das "An den Anfang gehen" in den vorherigen glyph reingehen, dann schreibe nur Pixel, die im neuen Glyph nicht Hintergrund sind
                            uint16_t suppressedPixels = p - g->startX;
                            p=g->startX;
                            p = g->WriteLineToBuffer(font, startline, l, p, buffer, colors, suppressedPixels);
                            ESP_LOGD(TAG, "glyph[%u] defines Line %d. Suppress %upx and write Bitmap. New pos is %upx", ghIndex, l, suppressedPixels, p);
                        }
                        else{
                            // wenn wir durch das "An den Anfang gehen" nicht unmittelbar an den vorherigen glyph anschließen, dann schreibe Hintergrund-Farbe
                            uint16_t p_temp0=p;
                            p = WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex, l, p, buffer);
                            uint16_t p_temp1=p;
                            p = g->WriteLineToBuffer(font, startline, l, p, buffer, colors, 0);
                            if(p_temp0!=p_temp1){
                                ESP_LOGD(TAG, "glyph[%u] defines Line %d. Write %u filler pixels till start and then bitmap with %upx. New pos is %upx", ghIndex, l, p_temp1-p_temp0, p-p_temp1, p);
                            }else{
                                ESP_LOGD(TAG, "glyph[%u] defines Line %u. Bitmap starts directly after previous. Write bitmap with %upx. New pos is %upx", ghIndex, l, p-p_temp1, p);
                            }
                        }
                    }
                    ghIndex++;
                }

                // Falls wir noch nicht am Ende der Line sind: Fülle ausgehend vom aktuellen p mit Hintergrund-Farbe
                p=WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(INT_MAX, l, p, buffer);
                assert(p==LINE_WIDTH_PIXELS);
                ESP_LOGD(TAG, "Line %d+%d with %u glyps ends", startline, l, glyphs.size());
            }
        }
    };
}
#undef TAG