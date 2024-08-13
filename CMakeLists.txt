cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

add_executable(template
        main.cpp
        )

project(template)

pico_sdk_init()

target_link_libraries(template PRIVATE
        pico_stdlib
        pico_audio_i2s
        hardware_adc
        hardware_flash
        )

# disable usb output, enable uart output
pico_enable_stdio_usb(template 0)
pico_enable_stdio_uart(template 1)

target_compile_definitions(template PRIVATE
        # compile time configuration of I2S
        PICO_AUDIO_I2S_MONO_INPUT=0
        PICO_AUDIO_I2S_MONO_OUTPUT=0
        USE_AUDIO_I2S=1
        )
# create map/bin/hex file etc.
pico_add_extra_outputs(template)