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
        //Die Render-Funktion fragt immer ganze Zeilen ab. Diese sorgt dafür, dass er Buffer ggf nicht vollständig genutzt werden könnte,
        //aber vereinfacht die Implementiertungen massiv!
        //Startline ist relativ zum angegebenen start-Point2D
        virtual void Render(uint16_t startline, uint16_t linesCount, uint16_t *buffer) = 0;
    };

}