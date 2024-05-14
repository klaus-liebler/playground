# One-Button Menu

One-Button Menu is a configurable hierachical settings menu system for esp-idf and common i2c or spi displays 

## Preamble
Ever wondered, what you can do with the strange "Boot" button on most esp32 dev boards.
Well, control settings of your device in a very comfortable way:-)

## Features (User Perspektive)
* Nice looking hence simple UI
* Individual tailored menu items for Integer, FixedPoint Decimals (bot internally stored as floats), Bool (yes, no), Options (select one of several options)
* Special "Placeholder"-Item to show static / dynamic Text
* Special "Confirmation Item" to trigger actions, e.g. "Load Values from Flash" or "Save Values to Flash"
* Hierachical; no level limit
* One Button controls it all (but a two/three/four button / rotary encoder setup is possible as well)
* Various Symbols and Icons help to understand, whats happening
* All Settings can be stored and loaded from flash using the ESP32 NVS libary


## Features (Integrator Perspektive)
* C++ and ESP-IDF
* No further dependencies
* Backends for common SSD1306 (I2C) and ST7789 (SPI) displays
* Smooth Hardware scrolling
* Double Buffered Rendering for 16bpp displays (not yet activated...)
* Advanced building process: node.js is used to transform glcd-Fonts, SVG symbols, TTF fonts to compact bitmap based binary representation and `#define`s for simple usage of symbols
* almost 500 Symbols are available, see https://pixelarticons.com/
* Callbacks available for all changes (examples provided)
* Text renderer supports center-aligned tabulator and right-aligned tabulator

## Features (Developer Perspective)
* Uses some c++20 features
* Uses some GoF-Patterns (e.g. adapter pattern)
* TTF Font File Parsing using opentype.js
* Unicode glyph handling
* Double Buffered Renderer for very fast rendering (data flow to display is not interrupted)
* Kerning Support (but makes no real sense on smaller displays and small fonts)
* Support for 4bpp and 1bpp fonts (basically each font type should render on all supported displays)
* many thanks to the folks @ LVGL! Your code really teached me, how to store and process glyphs. Hence, several lines of the code are somehow related to /stolen from [the LVGL project](https://github.com/lvgl/lvgl)
* Initialization code is stolen from https://lcamtuf.substack.com/p/mcu-land-part-9-st7789-the-final and the famous https://github.com/Bodmer/TFT_eSPI