# One-Button Menu

## Preamble
Ever wondered, what you can do with the strange "Boot" button on most esp32 dev boards.
Well, control settings of your device in a very comfortable way

## Features (User Perspektive)
* Nice looking hence simple UI
* One Button controls it all (but a two/three/four button / rotary encoder setup is possible as well)
* Unicode 


## Features (Integrator Perspektive)
* Backends for common SSD1306 and ST7789 displays
* Double Buffered Rendering for 16bpp displays (not yet activated...)
* Nice building process: node.js is used to transform glcd-Fonts, SVG symbols, TTF fonts to compact bitmap based binary representation and `#define`s for simple usage of symbols

## Features (Developer Perspective)
* Uses some c++20 features
* Uses some GoF-Patterns (e.g. adapter pattern)

* Unicode glyph handling