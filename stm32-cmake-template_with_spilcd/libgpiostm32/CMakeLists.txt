add_library(gpiostm32 src/gpioF0F4.cpp)
target_include_directories(gpiostm32
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ../app/include
    )
target_link_libraries(gpiostm32 
    HAL::STM32::F0::GPIO
    HAL::STM32::F0::LL_GPIO
    HAL::STM32::F0::CORTEX
    CMSIS::STM32::F072RB
)