add_library(spilcd16 src/printf.c src/printf.cc)

target_include_directories(spilcd16
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ../app/include
    )
target_link_libraries(spilcd16 
    HAL::STM32::F0::LL_SPI
    HAL::STM32::F0::LL_DMA
    HAL::STM32::F0::LL_UTILS
    CMSIS::STM32::F072RB
    gpiostm32
)