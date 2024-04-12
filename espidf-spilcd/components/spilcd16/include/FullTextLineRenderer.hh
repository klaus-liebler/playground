#pragma once
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <bit>
#include <esp_log.h>
#include "RGB565.hh"
#include "lvgl/lvgl.h"
#define TAG "LCD"
#include "interfaces.hh"
#include "lvgl.h"
#include "text_utils.hh"
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

            uint16_t startY; // mit Bezug zum topleft. ab dort beginnt das Bitmap; Y-Achse geht nach unten entsprechend Bildschirm
            uint8_t startX;  // mit bezug zum topleft, ab wo das bitmap startet
            bool BitmapDefinesLine(uint16_t l)
            {
                return !(l < g->startY || l >= g->startY + g->glyph_dsc->box_h);
            }

            /*
            suppressedPixelIfBackground: so viele Pixel werden im BUffer nicht überschreiben, wenn Sie nur Hintergrund sind (wichtig beim Kerning)
            */
            uint16_t WriteLineToBuffer(uint16_t l, uint16_t p, uint16_t *buffer, uint16_t suppressedPixelIfBackground = 0)
            {

                const uint8_t *bitmap = glyph_dsc->glyph_bitmap;
                assert(l >= startY);
                assert(p == startX);
                assert(font->dsc->bpp == 4)
                    uint32_t bo = glyph_dsc->bitmap_index;

                for (int x = 0; x < glyph_dsc->box_w; x++) // x läuft die Breite der Bitmap entland
                {
                    int indexInBitmap = l * glyph_dsc->box_w + x;
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
        std::vector<GlyphHelper> glyphs(16);

        bool getNextOverallLimitsAlreadySent{false};
        FullTextlineRenderer(const lv_font_t *font, Point2D topfleft, Color::Color565 foreground, Color::Color565 background) : font(font),
                                                                                                                                topfleft(topfleft)
        {
            for (int i = 0; i < 16; i++)
            {
                colors[i] = background.overlayWith(foreground, opa4_table[i]).toST7789_SPI_native();
            }
        }

        void PopulateGlyphs(char *chars)
        {
            uint8_t posX = PADDING_LEFT;
            int baselineY = (LINE_HEIGHT_PIXELS + font->line_height) * 0.5; // font soll in der Mitte der LINE stehen
            while (*chars)
            {
                uint32_t nextGlyphIndex = GetGlyphIndex(getCodepointAndAdvancePointer(&chars));
                int32_t kv = GetKerningValue(currentGlyphIndex, nextGlyphIndex);
                GlyphHelper gh = {};
                gh.glyph_dsc = GetGlyphDesc(currentGlyphIndex);
                gh.startX = posX + glyph_dsc->ofs_x;
                gh.startY = baselineY - glyph_dsc->ofs_y - gh->glyph_dsc->box_h;
                uint16_t endX = gh.startX + gh->glyph_dsc->box_w;
                if (endX >= LINE_WIDTH_PIXELS)
                {
                    break; // Damit ist sicher gestellt, dass man bei der Ausgabe keinerlei überprüfung machen muss, ob irgendwelche Grenzen überschritten werden -->einfach glyphs zeichnen und gut!
                }
                glyphs.push_back(gh);
                posX += ((glyph_dsc->adv_w + kv) + (1 << 3)) >> 4;
            }
        }

    public:
        static FullTextlineRenderer *printfl(const lv_font_t *font, uint8_t line, Color::Color565 foreground, Color::Color565 background, const char *format, ...)
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
            FullTextlineRenderer tr = new FullTextlineRenderer(&Roboto_regular, Point2D(PADDING_LEFT, line * LINE_HEIGHT_PIXELS), foreground, Color::YELLOW, chars);
            tr->PopulateGlyphs(chars);
            free(chars);
            return tr;
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
                GlyphHelper *g = &glyphs[ghIndex + 1];
                end = g->startX;
            }
            while (p < end)
            {
                buffer[l * LINE_WIDTH_PIXELS + p] = colors[0];
                p++;
            }
            return p;
        }

        void Render(uint16_t startline, uint16_t cnt, uint16_t *buffer) override
        {
            // es ist sicher gestellt, dass wir am Anfang der linie beginnen
            int ghIndex = 0;
            for (uint16_t l = startline; i < startline + cnt; l++)
            {
                uint16_t p = 0;
                while (ghIndex < glyphs.size())
                {
                    // Verarbeite eine Zeile eines glyph pro iteration
                    // Hole also den nächsten glyph
                    GlyphHelper *g = &glyphs[ghIndex];
                    if (!g->BitmapDefinesLine(l))
                    {
                        // Fall A: für diese Zeile exisitert keine Bitmap
                        // schreibe ausgehend vom aktuellen pixel (nicht vom eingetragenen startX, das gilt nur für Bitmap-lines) die Hintergrund-Farbe, und zwar bis zum Beginn des nächsten glyph bzw bis zum Ende der line
                        p = WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex + 1, p, &buffer);
                    }
                    else
                    {
                        // Fall B: für diese Zeile existieren Einträge in der Bitmap
                        // gehe an den Anfang des Bitmaps
                        if (p > g->startX)
                        {
                            // wenn wir durch das "An den Anfang gehen" in den vorherigen glyph reingehen, dann schreibe nur Pixel, die im neuen Glyph nicht Hintergrund sind
                            p = g->WriteLineToBuffer(l, p, buffer, p - g->startX);
                        }
                        // wenn wir durch das "An den Anfang gehen" nicht unmittelbar an den vorherigen glyph anschließen, dann schreibe Hintergrund-Farbe
                        p = WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex, p, &buffer);
                        p = g->WriteLineToBuffer(l, p, buffer, 0);
                    }
                    ghIndex++;
                }

                // Falls wir noch nicht am Ende der Line sind: Fülle ausgehend vom aktuellen p mit Hintergrund-Farbe
                WriteBackgroundTillStartOfGlyphBitmapOrEndOfLine(ghIndex + 1000, l, p, buffer);
            }
        }
    };
}