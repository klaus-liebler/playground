#pragma once
#include <cstdint>
#include <errorcodes.hh>
namespace display
{
    class FullLineWriter
    {
    public:
        virtual size_t printfl(int line, bool invert, const char *format, ...) = 0;
        virtual void ClearScreenAndResetStartline(bool invert = false, uint8_t start_textline_nominator = 0, uint8_t start_textline_denominator = 1) = 0;
        virtual void SetStartline(uint8_t startline) = 0;
        virtual void Scroll(int textLines) = 0;
        virtual uint8_t GetShownLines() = 0;
        virtual uint8_t GetAvailableLines() = 0;
    };



    class Point2D
    {
    public:
        int16_t x;
        int16_t y;
        void CopyFrom(Point2D &other)
        {
            this->x = other.x;
            this->y = other.y;
        }
        uint32_t PixelsTo(Point2D other)
        {
            return (std::max(0, other.x - x)) * (std::max(0, other.y - y));
        }
        Point2D(int16_t x, int16_t y) : x(x), y(y) {}
        Point2D() : x(0), y(0) {}
    };

    class Color565
    {
        private:
        const uint16_t native_value;
        public:
        Color565(uint8_t red, uint8_t green, uint8_t blue){
            native_value = (((red & 0b11111000)<<8) + ((green & 0b11111100)<<3) + (blue>>3));

        }

        Color565 overlayWith(Color565 overlay, uint8_t alpha0_255 ){
            
        }

        uint16_t toST7789_SPI_native(){
                        
            if(std::endian::native == std::endian::big) return native_value;
            else if(std::endian::native == std::endian::little) return std::byteswap(native_value);//ESP32 is little endian
            else return 0;
        }        
    };

    namespace Color{
        constexpr Color565 BLACK{Color565(0,0,0)};
        constexpr Color565 WHITE{Color565(255,255,255)};
        constexpr Color565 BLUE{Color565(0,0,255)};
        constexpr Color565 GREEN{Color565(0,255,0)};
        constexpr Color565 RED{Color565(255,0,0)};
        constexpr Color565 YELLOW{Color565(255,255,0)};
        constexpr Color565 SILVER{Color565(192,192,192)};
    }

    class IAsyncRenderer
    {
    public:
        /**
         * Methode liefert zurück, welcher Bereich auf dem LCD zu beschreiben ist.
         * Aufrufer muss sich dann darum kümmern, dass dieser Bereich auf durch eine entsprechende Anzahl an Render-Aufrufen auch wirklich beschrieben wird
         * Funktion returniert false, wenn es keine vollzuschreibenden Bereiche mehr gibt
         * bufferSizePixels ist lediglich informativ, damit der asyncRenderer sich bei Bedarf darauf optimieren kann
         */
        virtual bool GetNextOverallLimits(size_t bufferSizePixels, Point2D &start, Point2D &end_excl) = 0;
        // Die Render-Funktion fragt immer ganze Zeilen ab. Diese sorgt dafür, dass er Buffer ggf nicht vollständig genutzt werden könnte,
        // aber vereinfacht die Implementiertungen massiv!
        // Startline ist relativ zum angegebenen start-Point2D
        virtual void Render(uint16_t startline, uint16_t linesCount, uint16_t *buffer) = 0;
    };

    class IRendererHost{
        public:
        virtual ErrorCode Draw(IAsyncRenderer *renderer, bool considerOffsetsOfVisibleArea = true)=0;
        virtual ErrorCode prepareVerticalStrolling(uint16_t fixedTop, uint16_t fixedBottom)=0;
        virtual ErrorCode doVerticalStrolling(uint16_t lineOnTop_0_to_HEIGHT_OF_SCROLL_AREA)=0;
    };

}