cmake_minimum_required(VERSION 3.13)

#include(pico_sdk_import.cmake)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(blink LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

pico_sdk_init()

add_executable(blink
        main.cpp
        ssd1306_i2c.c
        mpu6050_i2c.c
        neo6m.cpp
        sim800l.cpp
        sleep_control.c
        )

# pull in common dependencies
target_link_libraries(blink
        pico_stdlib
        hardware_i2c
        hardware_rtc
        hardware_sleep
        hardware_clocks
        hardware_rosc)

# create map/bin/hex file etc.
pico_add_extra_outputs(blink)

set_property(TARGET blink PROPERTY CXX_STANDARD 11)

# add url via pico_set_program_url
#example_auto_set_url(blink)