/*
	Based on https://github.com/sumotoy/TFT_ILI9163C
	Based on https://github.com/adafruit/Adafruit-GFX-Library
*/
#pragma once

#include <gpiostm32/gpioF0F4.hpp>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "stm32f0xx_ll_spi.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_utils.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>
#include "spilcd16/printf.h"

static constexpr uint8_t SPI_LCD_DELAY_SIGN = 0x80;
constexpr uint8_t PARAM_BASE = 1;
constexpr uint8_t DELAY_0ms = 0x00;
constexpr uint8_t DELAY_10ms = 0x40;
constexpr uint8_t DELAY_150ms = 0x80;
constexpr uint8_t DELAY_500ms = 0xC0;

#define Swap2Bytes(val) ((((val) >> 8) & 0x00FF) | (((val) << 8) & 0xFF00))

constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t NAVY = 0x000F;		 /*   0,   0, 128 */
constexpr uint16_t DARKGREEN = 0x03E0;	 /*   0, 128,   0 */
constexpr uint16_t DARKCYAN = 0x03EF;	 /*   0, 128, 128 */
constexpr uint16_t MAROON = 0x7800;		 /* 128,   0,   0 */
constexpr uint16_t PURPLE = 0x780F;		 /* 128,   0, 128 */
constexpr uint16_t OLIVE = 0x7BE0;		 /* 128, 128,   0 */
constexpr uint16_t LIGHTGREY = 0xC618;	 /* 192, 192, 192 */
constexpr uint16_t DARKGREY = 0x7BEF;	 /* 128, 128, 128 */
constexpr uint16_t BLUE = 0x001F;		 /*   0,   0, 255 */
constexpr uint16_t GREEN = 0x07E0;		 /*   0, 255,   0 */
constexpr uint16_t CYAN = 0x07FF;		 /*   0, 255, 255 */
constexpr uint16_t RED = 0xF800;		 /* 255,   0,   0 */
constexpr uint16_t MAGENTA = 0xF81F;	 /* 255,   0, 255 */
constexpr uint16_t YELLOW = 0xFFE0;		 /* 255, 255,   0 */
constexpr uint16_t WHITE = 0xFFFF;		 /* 255, 255, 255 */
constexpr uint16_t ORANGE = 0xFD20;		 /* 255, 165,   0 */
constexpr uint16_t GREENYELLOW = 0xAFE5; /* 173, 255,  47 */
constexpr uint16_t PINK = 0xF81F;

/// Font data stored PER GLYPH
typedef struct
{
	uint16_t bitmapOffset; ///< Pointer into GFXfont->bitmap
	uint8_t width;		   ///< Bitmap dimensions in pixels
	uint8_t height;		   ///< Bitmap dimensions in pixels
	uint8_t xAdvance;	   ///< Distance to advance cursor (x axis)
	int8_t xOffset;		   ///< X dist from cursor pos to UL corner
	int8_t yOffset;		   ///< Y dist from cursor pos to UL corner, negative!!!
} GFXglyph;

/// Data stored for FONT AS A WHOLE
typedef struct
{
	uint8_t *bitmap;  ///< Glyph bitmaps, concatenated
	GFXglyph *glyph;  ///< Glyph array
	uint8_t first;	  ///< ASCII extents (first char)
	uint8_t last;	  ///< ASCII extents (last char)
	uint8_t yAdvance; ///< Newline distance (y axis)
} GFXfont;

enum class DisplayRotation : uint8_t
{
	//	ROT0    =0b00000000,
	//	ROT90CW =0b01100000,
	//	ROT180CW=0b11000000,
	//	ROT270CW=0b10100000,
	ROT0 = 0,
	ROT90CW = 1,
	ROT180CW = 2,
	ROT270CW = 3,
};

enum class BufferfillerMode : uint8_t
{
	NONE,
	RECT,
	STRING,
};

enum class Anchor : uint8_t
{
	TOP_LEFT,
	BOTTOM_LEFT,
	TOP_RIGHT,
	BOTTOM_RIGHT,
};

enum class PrintStringError
{
	OK = 0,
	PARAM_ASEERTION_ERROR,
	LAYOUT_NOT_IMPLEMENTED,
	BUFFER_TOO_SMALL,
	NO_FONT_SET,
	TOO_LONG,
};

class DotMatrixDisplay{
	virtual void backlight(bool on)=0;
	virtual PrintStringError printString(int16_t cursor_x, int16_t cursor_y, int16_t xWindowStart, int16_t xWindowEnd, int16_t yWindowStart, int16_t yWindowEnd, Anchor anchorType, const char *format, ...)=0;
	virtual void begin(void)=0;
	virtual void fillScreen()=0;
	virtual void printString(int16_t cursor_x, int16_t cursor_y, Anchor anchorType, const char *format, ...) =0;
	virtual void drawPixel(int16_t x, int16_t y)=0;
	virtual void drawFastVLine(int16_t x, int16_t y, int16_t h)=0;
	virtual void drawFastHLine(int16_t x, int16_t y, int16_t w)=0;
	virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h)=0;
	virtual void setColors(uint16_t foregroundColor, uint16_t backgroundColor)=0;
	virtual void setFont(const GFXfont* font)=0;
	virtual void idleMode(bool onOff) = 0;
	virtual void displayOn(bool onOff) = 0;
	virtual void sleepMode(bool mode) = 0;
};

template <uint32_t dmaChannel, Pin _cs, Pin _dc, Pin _backlight, Pin _rst, size_t STRING_BUFFER_SIZE_CHARS, size_t BUFFER_SIZE_BYTES> //dc HIGH -->DATA>
class SPILCD16:public DotMatrixDisplay
{

public:
	SPILCD16(SPI_TypeDef *spi, const DisplayRotation rotation) : spi(spi),  rotation(rotation){
	}
	virtual ~SPILCD16(){};

	void backlight(bool on)
	{
		Gpio::Set(_backlight, on);
	}
	PrintStringError printString(int16_t cursor_x, int16_t cursor_y, int16_t xWindowStart, int16_t xWindowEnd, int16_t yWindowStart, int16_t yWindowEnd, Anchor anchorType, const char *format, ...) {
		//X und Y definieren eine Ankerposition. In welche Richtung ab dort der Text geschrieben wird, bestimmt anchorType
		if(!this->font) return PrintStringError::NO_FONT_SET;
		if(anchorType!=Anchor::BOTTOM_LEFT && anchorType!=Anchor::BOTTOM_RIGHT) return PrintStringError::LAYOUT_NOT_IMPLEMENTED;
		if(xWindowEnd<=xWindowStart) return PrintStringError::PARAM_ASEERTION_ERROR;
		if(yWindowEnd<=yWindowStart) return PrintStringError::PARAM_ASEERTION_ERROR;
		if(BUFFER_SIZE_BYTES/2 < this->_width) return PrintStringError::BUFFER_TOO_SMALL;


		while (this->bufferFillerMode != BufferfillerMode::NONE)
					__NOP(); //wait till a previous process has been finished;



		this->xWindowStart=std::max((int16_t)0, xWindowStart);
		this->xWindowEnd=std::min((int16_t)_width,xWindowEnd);
		this->yWindowStart=std::max((int16_t)0,yWindowStart);
		this->yWindowEnd=std::min((int16_t)_height, yWindowEnd);
		this->bufferStep = yWindowEnd-yWindowStart;
		va_list va;
		va_start(va, format);
		this->strLength=vsnprintf(strBuffer, STRING_BUFFER_SIZE_CHARS, format, va);
		va_end(va);
		if(anchorType==Anchor::BOTTOM_LEFT)
		{
			this->cursor_x = cursor_x;
			this->cursor_y = cursor_y;
		}
		else if(anchorType==Anchor::BOTTOM_RIGHT)
		{
			this->cursor_x = cursor_x-getTextPixelLength(strBuffer);
			this->cursor_y = cursor_y;
		}


		setAddr(xWindowStart, yWindowStart, xWindowEnd-1, yWindowEnd-1);
		this->bufferFillerMode = BufferfillerMode::STRING;


		while (LL_SPI_IsActiveFlag_BSY(spi)) //as long as the setAddr last byte is transmitted
				__NOP();
		Gpio::Set(_dc, true);
		Gpio::Set(_cs, false);
		this->printStringCb();
		return PrintStringError::OK;
	}
	void begin(void)
	{
		//bool initialValue, OutputType ot = OutputType::PUSH_PULL, OutputSpeed speed = OutputSpeed::LOW, PullDirection pulldir = PullDirection::NONE
		Gpio::ConfigureGPIOOutput(_backlight, false, OutputType::PUSH_PULL, OutputSpeed::HIGH);
		Gpio::ConfigureGPIOOutput(_dc, false, OutputType::PUSH_PULL, OutputSpeed::HIGH);
		Gpio::ConfigureGPIOOutput(_cs, false, OutputType::PUSH_PULL, OutputSpeed::HIGH);
		if (_rst != Pin::NO_PIN)
		{
			Gpio::ConfigureGPIOOutput(_rst, false, OutputType::PUSH_PULL, OutputSpeed::HIGH);
			LL_mDelay(3); //min 10us according to datasheet
			Gpio::Set(_rst, true);
			LL_mDelay(150); //min 120ms according to datasheet
		}
		LL_DMA_ConfigAddresses(
			DMA1, dmaChannel, (uint32_t)buffer, LL_SPI_DMA_GetRegAddr(spi),
			LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
		LL_SPI_Enable(spi);
		chipInit();
	}
	void fillScreen()
	{
		int px;
		setAddr(0x00, 0x00, _width, _height); //go home
		for (px = 0; px < _width * _height; px++)
		{
			writedata16(foregroundColor);
		}
	}
	void printString(int16_t cursor_x, int16_t cursor_y, Anchor anchorType, const char *format, ...) {
		//X und Y definieren eine Ankerposition. In welche Richtung ab dort der Text geschrieben wird, bestimmt anchorType
		if(!this->font) return;
		if(anchorType!=Anchor::BOTTOM_LEFT) while (1) __asm__ __volatile__ ("bkpt #0"); //NOT Implemented


		while (this->bufferFillerMode != BufferfillerMode::NONE)
					__NOP(); //wait till a previous process has been finished;
		if(BUFFER_SIZE_BYTES/2 < this->_width)//as we write line by line, we need at least one line in the buffer
		{
			while (1) {
					__asm__ __volatile__ ("bkpt #0");
				}
		}


		va_list va;
		va_start(va, format);
		this->strLength=vsnprintf(strBuffer, STRING_BUFFER_SIZE_CHARS, format, va);
		va_end(va);

		int16_t x1, y1, x2, y2;

		getTextBounds(strBuffer, cursor_x, cursor_y, anchorType, &x1, &y1, &x2, &y2);
		this->cursor_x = cursor_x;
		this->cursor_y = cursor_y;
		this->xWindowStart=std::max((int16_t)0, x1);
		this->xWindowEnd=std::min((int16_t)_width,x2);
		this->yWindowStart=std::max((int16_t)0,y1);
		this->yWindowEnd=std::min((int16_t)_height, y2);


		//this->distanceToBaseline = y1-cursor_y; //negativer Wert, wie glyph->yoffset

		setAddr(xWindowStart, yWindowStart, xWindowEnd-1, yWindowEnd-1);
		this->bufferFillerMode = BufferfillerMode::STRING;
		this->bufferStep = y2-y1;

		while (LL_SPI_IsActiveFlag_BSY(spi)) //as long as the setAddr last byte is transmitted
				__NOP();
		Gpio::Set(_dc, true);
		Gpio::Set(_cs, false);
		this->printStringCb();
		return;
	}
	void printStringCb() {
		uint16_t *buffer16 = (uint16_t *) buffer;
		uint16_t length = 0;
		size_t cidx = 0;
		for(int16_t u=0;u<cursor_x-xWindowStart;u++)
		{
			buffer16[length] = this->backgroundColor;
			length++;
		}
		while(cidx<strLength)
		{
			uint8_t c = strBuffer[cidx];
			GFXglyph *glyph  = &(font->glyph[c-font->first]);
			uint8_t  *bitmap = font->bitmap;
			uint16_t bo = glyph->bitmapOffset;
			uint8_t  w  = glyph->width, //of bitmap
					h  = glyph->height; //of bitmap
			int8_t   xo = glyph->xOffset, //of bitmap
					yo = glyph->yOffset, //negativ!!
						adv = glyph->xAdvance;


			//BufferStep gibt an, wie viele Zeilen noch fehlen bis zur letzten Zeile

			for(int16_t xx=0; xx<adv;xx++)
			{
				volatile bool bit=false;
				//Prüfung, ob wir innerhalb der Bitmap sind
				int16_t distanceToBaseline= this->yWindowEnd - this->bufferStep - this->cursor_y;
				if(xx>=xo && xx<xo+w && distanceToBaseline >= yo && distanceToBaseline < yo+h)
				{
					volatile int line_in_bitmap = distanceToBaseline-yo;
					int bitindex = line_in_bitmap*w+(xx-xo);
					int byteindex = bitindex >> 3;
					int bitinbyte = bitindex & 0x7;
					bit = (bitmap[bo+byteindex] << bitinbyte) & 0x80;
				}
				buffer16[length] = bit?this->foregroundColor:this->backgroundColor;
				length++;
			}



			cidx++;
		}
		while(length<(xWindowEnd-xWindowStart))
		{
			buffer16[length] = this->backgroundColor;
			length++;
		}

		this->bufferStep--;
		length *= 2;
		LL_DMA_SetDataLength(DMA1, dmaChannel, length);
		LL_DMA_EnableChannel(DMA1, dmaChannel);
	}
	void drawPixel(int16_t x, int16_t y){
		if (boundaryCheck(x, y))
			return;
		if ((x < 0) || (y < 0))
			return;
		setAddr(x, y, x, y);
		//TODO: Version von SUMOTOY setAddr(x, y, x + 1, y + 1);
		writedata16(this->foregroundColor);
	}
	void drawFastVLine(int16_t x, int16_t y, int16_t h){
		// Rudimentary clipping
		if (boundaryCheck(x, y))
			return;
		if (((y + h) - 1) >= _height)
			h = _height - y;
		setAddr(x, y, x, (y + h) - 1);
		while (h-- > 0)
		{
			writedata16(this->foregroundColor);
		}
	}
	void drawFastHLine(int16_t x, int16_t y, int16_t w) {
		// Rudimentary clipping
		if (boundaryCheck(x, y))
			return;
		if (((x + w) - 1) >= _width)
			w = _width - x;
		setAddr(x, y, (x + w) - 1, y);
		while (w-- > 0) {

			writedata16(foregroundColor);
		}

	}
	
	void fillRect(int16_t x, int16_t y, int16_t w, int16_t h) {
		if (boundaryCheck(x, y))
			return;
		if (((x + w) - 1) >= _width)
			w = _width - x;
		if (((y + h) - 1) >= _height)
			h = _height - y;

		while (this->bufferFillerMode != BufferfillerMode::NONE)
			__NOP(); //wait till a previous process has been finished;
		setAddr(x, y, (x + w) - 1, (y + h) - 1);
		this->bufferFillerMode = BufferfillerMode::RECT;
		this->bufferStep = w * h;
		while (LL_SPI_IsActiveFlag_BSY(spi)) //as long as the setAddr last byte is transmitted
				__NOP();
		Gpio::Set(_dc, true);
		Gpio::Set(_cs, false);
		this->fillRectCb();

	//	for (y = h; y > 0; y--) {
	//		for (x = w; x > 0; x--) {
	//			writedata16(color);
	//		}
	//	}

	}
	//bufferStep zählt, wie viele 16bit-Farben noch zu senden sind
	void fillRectCb() {
		size_t length = 0;
		uint16_t *buffer16 = (uint16_t *) buffer;
		while (bufferStep > 0 && length < BUFFER_SIZE_BYTES / 2) {
			buffer16[length] = this->foregroundColor;
			length++;
			bufferStep--;
		}
		length *= 2;
		LL_DMA_SetDataLength(DMA1, dmaChannel, length);
		LL_DMA_EnableChannel(DMA1, dmaChannel);
	}
	void spiTxCompleteCallback(){
		LL_DMA_DisableChannel(DMA1, this->dmaChannel);
		if (this->bufferStep == 0)
		{
			Gpio::Set(_cs, true);
			this->bufferFillerMode = BufferfillerMode::NONE;
			return;
		}

		switch (this->bufferFillerMode)
		{
		case BufferfillerMode::RECT:
			this->fillRectCb();
			break;
		case BufferfillerMode::STRING:
			this->printStringCb();
			break;
		default:
			return;
		}
	}
	void setColors(uint16_t foregroundColor, uint16_t backgroundColor){
		this->foregroundColor= Swap2Bytes(foregroundColor); //Because of DMA byte order
		this->backgroundColor= Swap2Bytes(backgroundColor);
	}
	void setFont(const GFXfont* font){
		this->font=font;
	}
	bool DMAIdle()
	{
		return this->bufferFillerMode == BufferfillerMode::NONE;
	}
	virtual void idleMode(bool onOff) = 0;
	virtual void displayOn(bool onOff) = 0;
	virtual void sleepMode(bool mode) = 0;
protected:
	void processInitCommands(const uint8_t *addr)
	{

		uint8_t numCommands, numArgs;
		uint8_t ms;

		numCommands = *addr++; // Number of commands to follow
		while (numCommands--)  // For each command...
		{
			writecommand(*addr++);			   // Read, issue command
			numArgs = *addr++;				   // Number of args to follow
			ms = numArgs & SPI_LCD_DELAY_SIGN; // If hibit set, delay follows args
			numArgs &= ~SPI_LCD_DELAY_SIGN;	   // Mask out delay bit
			while (numArgs--)				   // For each argument...
			{
				writedata(*addr++); // Read, issue argument
			}

			if (ms)
			{
				ms = *addr++; // Read post-command delay time (ms)
				LL_mDelay(2 * ms);
			}
		}
	}
	void processInitCommandsCompact(const uint8_t *addr)
	{

		uint8_t numArgs = *addr++;
		uint16_t delay = 0;
		static constexpr uint16_t lookup[] = {0, 10, 150, 500};
		while (numArgs != 0) // For each command...
		{
			delay = (numArgs & 0xC0);
			numArgs &= 0x3F;	   //Max 64 Args
			writecommand(*addr++); // Read, issue command
			while (--numArgs)	   // For each argument...
			{
				writedata(*addr++); // Read, issue argument
			}
			if (delay)
			{
				delay >>= 6;
				LL_mDelay(lookup[delay]);
			}
			numArgs = *addr++;
		}
	}
	void writecommand(uint8_t c)
	{
		while (LL_SPI_IsActiveFlag_BSY(spi))
			__NOP();
		Gpio::Set(_dc, false);
		Gpio::Set(_cs, false);
		LL_SPI_TransmitData8(spi, c);

		Gpio::Set(_cs, true);
	}
	void writedata(uint8_t c)
	{

		while (LL_SPI_IsActiveFlag_BSY(spi))
			__NOP();
		Gpio::Set(_dc, true);
		Gpio::Set(_cs, false);
		LL_SPI_TransmitData8(spi, c);
		Gpio::Set(_cs, true);
	}
	void writedataMulti(uint8_t *c, size_t len)
	{

		while (LL_SPI_IsActiveFlag_BSY(spi))
			__NOP();
		Gpio::Set(_dc, true);
		Gpio::Set(_cs, false);
		for(size_t i=0;i<len;i++){
			LL_SPI_TransmitData8(spi, c[i]);
		}
		Gpio::Set(_cs, true);
	}
	void writedata16(uint16_t d)
	{

		while (LL_SPI_IsActiveFlag_BSY(spi))
			__NOP();
		Gpio::Set(_dc, true);
		Gpio::Set(_cs, false);
		uint8_t d1 = (uint8_t)(d >> 8);
		LL_SPI_TransmitData8(spi, d1);
		uint8_t d2 = (0xFF) & d;
		while (!LL_SPI_IsActiveFlag_TXE(spi))
			;
		LL_SPI_TransmitData8(spi, d2);
		Gpio::Set(_cs, true);
	}
	virtual void chipInit() = 0;
	virtual void setAddr(uint16_t x_min_incl, uint16_t y_min_incl, uint16_t x_max_incl, uint16_t y_max_incl) = 0;
	uint16_t _width = 0, _height = 0;
	bool sleep = false;
private:
/*
    @brief    Helper to determine size of a character with current font/size.
    @param    c     The ascii character in question
    @param    x     Pointer to x location of character
    @param    y     Pointer to y location of character
*/
	void charBounds(char c, int16_t *x) {
		if(c == '\n') { // Newline?
			*x  = 0;    // Reset x to zero, advance y by one line
			return;
		}
		if(c == '\r') { // Not a carriage return; is normal char
			return;
		}
		uint8_t first = font->first;
		uint8_t last  = font->last;
		if((c < first) && (c > last)) { // Char present in this font?
			return;
		}
		GFXglyph *glyph = &((font->glyph)[c - first]);
		*x += glyph->xAdvance;
	}
	int16_t getTextPixelLength(const char *str) {

		int16_t x2=0;

		size_t l = strlen(str);
		for(size_t i=0;i<l; i++)
		{
			char c = str[i];
			if(c == '\n' || c == '\r') { // Newline?
				continue;
			}

			uint8_t first = font->first;
			uint8_t last  = font->last;
			if((c < first) && (c > last)) { // Char present in this font?
				continue;
			}
			GFXglyph *glyph = &((font->glyph)[c - first]);
			x2 += glyph->xAdvance;

		}
		return x2;
	}
	void getTextBounds(const char *str, int16_t x, int16_t y, Anchor anchorType, int16_t *x1, int16_t *y1, int16_t *x2, int16_t *y2) {
		if(anchorType!=Anchor::BOTTOM_LEFT) while (1) __asm__ __volatile__ ("bkpt #0"); //NOT Implemented

		*x1 = x;
		*x2 = x;
		*y2 = y;
		*y1 = y;

		size_t l = strlen(str);
		for(size_t i=0;i<l; i++)
		{
			char c = str[i];
			if(c == '\n' || c == '\r') { // Newline?
				continue;
			}

			uint8_t first = font->first;
			uint8_t last  = font->last;
			if((c < first) && (c > last)) { // Char present in this font?
				continue;
			}
			GFXglyph *glyph = &((font->glyph)[c - first]);
			*x2 += glyph->xAdvance;
			*y1 = std::min((int16_t)*y1, (int16_t)(y+glyph->yOffset));
			*y2 = std::max((int16_t)*y1, (int16_t)(y+glyph->yOffset+glyph->height));
		}
	}
	bool boundaryCheck(int16_t x, int16_t y) {
		if ((x >= _width) || (y >= _height))
			return true;
		return false;
	}
	
	SPI_TypeDef *spi;
	const DisplayRotation rotation;
	

	int16_t cursor_x = 0, cursor_y = 0;
	char strBuffer[STRING_BUFFER_SIZE_CHARS];
	size_t strLength = 0;
	int16_t xWindowStart = 0, xWindowEnd = 0, yWindowStart = 0, yWindowEnd = 0;
	__IO uint8_t buffer[BUFFER_SIZE_BYTES] __attribute__((aligned(4)));
	__IO int32_t bufferStep = 0; //kann vom bufferFiller genutzt werden, um sich zu merken, wo beim nächsten Callback weiterzumachen ist
	BufferfillerMode bufferFillerMode = BufferfillerMode::NONE;
	uint16_t foregroundColor = BLACK;
	uint16_t backgroundColor = WHITE;
	const GFXfont *font = NULL;
};
