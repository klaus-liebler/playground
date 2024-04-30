#pragma once
#include <cstdint>
namespace lcd_i2c_1bpp
{
    namespace CMD
    {
        constexpr uint8_t WRITE_CMD{0x00};
        constexpr uint8_t WRITE_CMD_STREAM{0x00};
        constexpr uint8_t WRITE_DAT{0x40};
        constexpr uint8_t MEMORYMODE{0x20};
        constexpr uint8_t MEMORYMODE_PARAM_HORIZONTAL{0x0};
        constexpr uint8_t MEMORYMODE_PARAM_VERTICAL{0x1};
        constexpr uint8_t MEMORYMODE_PARAM_PAGE{0x2};

        constexpr uint8_t SET_COLUMN_ADDRESS_RANGE{0x21};      ///< See datasheet
        constexpr uint8_t SET_PAGE_ADDRESS_RANGE{0x22};          ///< See datasheet
        constexpr uint8_t SETSTARTLINE{0x40};             ///< See datasheet
        constexpr uint8_t SETCONTRAST{0x81};              ///< See datasheet
        constexpr uint8_t CHARGEPUMP{0x8D};               ///< See datasheet
        constexpr uint8_t CHARGEPUMP_PARAM_ENABLE{0x14};  ///< See datasheet
        constexpr uint8_t CHARGEPUMP_PARAM_DISABLE{0x10}; ///< See datasheet
        constexpr uint8_t SEGREMAP__0{0xA0};              ///< See datasheet
        constexpr uint8_t SEGREMAP__1{0xA1};              ///< See datasheet
        constexpr uint8_t DISPLAYALLON_RESUME{0xA4};      ///< See datasheet
        constexpr uint8_t DISPLAYALLON{0xA5};             ///< Not currently used
        constexpr uint8_t NORMALDISPLAY{0xA6};            ///< See datasheet
        constexpr uint8_t INVERTDISPLAY{0xA7};            ///< See datasheet
        constexpr uint8_t SETMULTIPLEX{0xA8};             ///< See datasheet
        constexpr uint8_t DISPLAYOFF{0xAE};               ///< See datasheet
        constexpr uint8_t DISPLAYON{0xAF};                ///< See datasheet
        constexpr uint8_t COMSCAN__INC{0xC0};             ///< Not currently used
        constexpr uint8_t COMSCAN__DEC{0xC8};             ///< See datasheet
        constexpr uint8_t SETDISPLAYOFFSET{0xD3};         ///< See datasheet
        constexpr uint8_t SETDISPLAYCLOCKDIV{0xD5};       ///< See datasheet
        constexpr uint8_t SETPRECHARGE{0xD9};             ///< See datasheet
        constexpr uint8_t SETCOMPINS{0xDA};               ///< See datasheet
        constexpr uint8_t SETVCOMDETECT{0xDB};            ///< See datasheet

        // Scroll stuff
        constexpr uint8_t RIGHT_HORIZONTAL_SCROLL{0x26};              ///< Init rt scroll
        constexpr uint8_t LEFT_HORIZONTAL_SCROLL{0x27};               ///< Init left scroll
        constexpr uint8_t VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL{0x29}; ///< Init diag scroll
        constexpr uint8_t VERTICAL_AND_LEFT_HORIZONTAL_SCROLL{0x2A};  ///< Init diag scroll
        constexpr uint8_t DEACTIVATE_SCROLL{0x2E};                    ///< Stop scroll
        constexpr uint8_t ACTIVATE_SCROLL{0x2F};                      ///< Start scroll
        constexpr uint8_t SET_VERTICAL_SCROLL_AREA{0xA3};             ///< Set scroll range
    }
}