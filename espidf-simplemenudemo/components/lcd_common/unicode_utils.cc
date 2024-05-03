#include "unicode_utils.hh"
uint32_t unicode_utils::getCodepointAndAdvancePointer(char **c)
    {
        //if((**c)==0) return 0; not necessary; is first case!
        uint32_t codepoint{0};
        if (((**c) & 0b10000000) == 0)
        { // 1byte
            codepoint = **c;
            (*c) += 1;
        }
        else if (((**c) & 0b11100000) == 0b11000000)
        { // 2byte
            codepoint = (**c) & 0b00011111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
        }
        else if (((**c) & 0b11110000) == 0b11100000)
        { // 3byte
            codepoint = (**c) & 0b00001111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
        }
        else if (((**c) & 0b11111000) == 0b11110000)
        { // 4byte
            codepoint = (**c) & 0b00000111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
            codepoint <<= 6;
            (*c)++;
            codepoint |= (**c) & 0b00111111;
            (*c)++;
        }
        // ESP_LOGI(TAG, "Found codepoint %lu", codepoint);
        return codepoint;
    }