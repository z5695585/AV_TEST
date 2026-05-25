#include "gpio_drv.h"
static uint8_t s_pin_val[32];
static uint8_t s_pin_mode[32];

void gpio_init(uint8_t pin, uint8_t mode) { s_pin_mode[pin]=mode; }
uint8_t gpio_read(uint8_t pin) { return s_pin_val[pin]; }
void gpio_write(uint8_t pin, uint8_t level) { s_pin_val[pin]=level; }
void mock_gpio_set_pin(uint8_t pin, uint8_t val) { s_pin_val[pin]=val; }
uint8_t mock_gpio_get_pin_mode(uint8_t pin) { return s_pin_mode[pin]; }
uint8_t mock_gpio_get_pin_out(uint8_t pin) { return s_pin_val[pin]; }
