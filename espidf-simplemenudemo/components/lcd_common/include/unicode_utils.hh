#pragma once
#include <cstdint>
namespace unicode_utils
{
    uint32_t getCodepointAndAdvancePointer(char **c);
}