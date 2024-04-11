#pragma once
#include <cstdint>
namespace SPILCD16
{
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
         */
        virtual bool GetNextOverallLimits(Point2D &start, Point2D &end_excl) = 0;
        virtual void Render(uint32_t startPixel, uint16_t *buffer, size_t len) = 0;
    };

}