#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <bit>
#include "RGB565.hh"
#include "interfaces.hh"
#include "lcd_font.hh"
#include "unicode_utils.hh"
#define TAG "FTLR"
#include <esp_log.h>
using namespace display;
namespace spilcd16
{
    template <uint8_t LINE_HEIGHT_PIXELS, uint8_t LINE_WIDTH_PIXELS, uint8_t PADDING_LEFT, uint8_t PADDING_RIGHT>
    class FullTextlineRenderer : public IAsyncRenderer
    {
    private:
        const lcd_common::FontDesc* const font;
        Point2D topfleft; // ist absolute Bildschirmkoordinate obere linke Ecke
        uint16_t colors[16];
        std::vector<lcd_common::GlyphHelper> glyphs;
        bool getNextOverallLimitsAlreadySent{false};
    public:
        
        FullTextlineRenderer(const lcd_common::FontDesc* const font) : font(font)
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
            ESP_LOGD(TAG, "Printing '%s' to line %d", chars, line);
            
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
            lcd_common::GlyphHelper::PopulateGlyphs(font, chars, glyphs, PADDING_LEFT, LINE_WIDTH_PIXELS, PADDING_RIGHT);
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

        /*
        returns new absolute position in pixelline
        */
        size_t WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(int ghIndex, uint16_t *buffer_start_of_pixelline, uint16_t pos_in_pixelline)
        {
            size_t end = LINE_WIDTH_PIXELS;
            if (ghIndex < glyphs.size())
            {
                lcd_common::GlyphHelper *g = &glyphs[ghIndex];
                end = g->startX;
            }
            while (pos_in_pixelline < end)
            {
                buffer_start_of_pixelline[pos_in_pixelline] = colors[0];
                pos_in_pixelline++;
            }
            return pos_in_pixelline;
        }

        void Render(uint16_t startline, uint16_t cnt, uint16_t *buffer) override{

            const uint8_t MOVE_GLYPHS_DOWN{2};
            // es ist sicher gestellt, dass wir am Anfang der linie beginnen
            for (uint16_t l = 0; l < cnt; l++)
            {
                //startline bezieht sich relativ auf die obere linke ecke des rects, das gerade beschrieben wird
                //cnt bezieht sich auf die anzahl an Zeilen, die zu schreiben sind
                //das "l" meint hier die relative Zeile innerhalb des aktuellen buffersegments
                ESP_LOGD(TAG, "Line %d+%d with %u glyps starts", startline, l, glyphs.size());
                int ghIndex = 0;
                uint16_t* buffer_start_of_pixelline = &buffer[l*LINE_WIDTH_PIXELS];
                uint16_t pos_in_pixelline = WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex, buffer_start_of_pixelline, 0);
                ESP_LOGD(TAG, "Line %d+%d: Fill background till pos_in_pixelline=%upx", startline, l, pos_in_pixelline);
                while (ghIndex < glyphs.size())
                {
                    // Verarbeite eine Zeile eines glyph pro iteration
                    // Hole also den nächsten glyph
                    lcd_common::GlyphHelper *g = &glyphs[ghIndex];
                    
                    // gehe an den Anfang des Bitmaps
                    if (pos_in_pixelline > g->startX)
                    {
                        // wenn wir durch das "An den Anfang gehen" in den vorherigen glyph reingehen, dann schreibe nur Pixel, die im neuen Glyph nicht Hintergrund sind
                        uint16_t suppressedPixels = pos_in_pixelline - g->startX;
                        pos_in_pixelline=g->startX;
                        pos_in_pixelline+= g->WriteGlyphLineToBuffer16bpp(font, startline+l-MOVE_GLYPHS_DOWN, &buffer_start_of_pixelline[pos_in_pixelline], colors, suppressedPixels);
                        ESP_LOGD(TAG, "Line %d+%d: glyph[%u]: Suppress %upx and write Bitmap. pos_in_pixelline=%upx", startline, l, ghIndex, suppressedPixels, pos_in_pixelline);
                    }
                    else{
                        // wenn wir durch das "An den Anfang gehen" nicht unmittelbar an den vorherigen glyph anschließen, dann schreibe Hintergrund-Farbe
                        uint16_t p_temp0=pos_in_pixelline;
                        pos_in_pixelline = WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex, buffer_start_of_pixelline, pos_in_pixelline);
                        uint16_t p_temp1=pos_in_pixelline;
                        pos_in_pixelline+= g->WriteGlyphLineToBuffer16bpp(font, startline+l-MOVE_GLYPHS_DOWN, &buffer_start_of_pixelline[pos_in_pixelline], colors, 0);
                        if(p_temp0!=p_temp1){
                            ESP_LOGD(TAG, "Line %d+%d: Write filler pixels till pos_in_pixelline=%upx and bitmap till pos_in_pixelline=%upx", startline, l, p_temp1, pos_in_pixelline);
                        }else{
                            ESP_LOGD(TAG, "Line %d+%d: Bitmap starts directly.Write bitmap till pos_in_pixelline=%upx", startline, l, pos_in_pixelline);
                        }
                    }
                    ghIndex++;
                }

                // Falls wir noch nicht am Ende der Line sind: Fülle ausgehend vom aktuellen p mit Hintergrund-Farbe
                pos_in_pixelline=WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(INT_MAX, buffer_start_of_pixelline, pos_in_pixelline);
                assert(pos_in_pixelline==LINE_WIDTH_PIXELS);
                ESP_LOGD(TAG, "Line %d+%d with %u glyps ends", startline, l, glyphs.size());
            }
        }
    };
}
#undef TAG