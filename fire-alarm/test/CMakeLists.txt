cmake_minimum_required(VERSION 3.16)
project(fire-alarm-test C)

# Host-side test build
add_executable(fire-alarm-test
    test_main.c
    test_pattern_table.c
    test_alarm_sound.c
    test_alarm_light.c
    test_protocol.c

    # Mock drivers
    mock/mock_pwm.c
    mock/mock_gpio.c
    mock/mock_uart.c
    mock/mock_timer.c

    # Modules under test
    ../src/app/pattern_engine.c
    ../src/app/alarm_sound.c
    ../src/app/alarm_light.c
    ../src/app/test_protocol.c
    ../src/config/pattern_table.c
)

target_include_directories(fire-alarm-test PRIVATE
    ..
    mock
    ../src/driver
    ../src/app
    ../src/config
)

add_test(NAME fire-alarm-unit COMMAND fire-alarm-test)
