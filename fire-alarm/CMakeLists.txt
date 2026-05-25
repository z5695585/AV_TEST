cmake_minimum_required(VERSION 3.16)
project(fire-alarm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# HC32L110 DDL SDK path
set(HC32_SDK_DIR "${CMAKE_SOURCE_DIR}/../HC32L110_DDL_Rev1.3.2" CACHE PATH "HC32L110 SDK root")

# --- SDK library ---
set(HC32_SDK_SRCS
    ${HC32_SDK_DIR}/driver/src/ddl.c
    ${HC32_SDK_DIR}/driver/src/gpio.c
    ${HC32_SDK_DIR}/driver/src/uart.c
    ${HC32_SDK_DIR}/driver/src/adt.c
    ${HC32_SDK_DIR}/driver/src/clk.c
    ${HC32_SDK_DIR}/driver/src/interrupts_hc32l110.c
)

add_library(hc32_sdk STATIC
    ${HC32_SDK_SRCS}
    ${HC32_SDK_DIR}/mcu/common/system_hc32l110.c
)

target_include_directories(hc32_sdk PUBLIC
    ${HC32_SDK_DIR}/driver/inc
    ${HC32_SDK_DIR}/mcu/common
    ${HC32_SDK_DIR}/mcu/CMSIS/Core/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hal/hc32l110
)

# --- HAL layer ---
add_library(driver_hal STATIC
    src/hal/hc32l110/pwm_drv.c
    src/hal/hc32l110/gpio_drv.c
    src/hal/hc32l110/uart_drv.c
    src/hal/hc32l110/timer_drv.c
)

target_include_directories(driver_hal PUBLIC src/driver)
target_link_libraries(driver_hal hc32_sdk)

# --- App layer ---
add_library(app STATIC
    src/app/pattern_engine.c
    src/app/alarm_sound.c
    src/app/alarm_light.c
    src/app/test_protocol.c
    src/config/pattern_table.c
)

target_include_directories(app PUBLIC src/app src/config src/driver)
target_link_libraries(app driver_hal)

# --- Target executable ---
add_executable(fire-alarm.elf
    src/main.c
    ${HC32_SDK_DIR}/mcu/GCC/startup_hc32l110.s
)

target_link_libraries(fire-alarm.elf app driver_hal hc32_sdk)

# Host test build
enable_testing()
add_subdirectory(test)
