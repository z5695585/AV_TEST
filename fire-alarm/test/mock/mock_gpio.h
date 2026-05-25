#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H
#include <stdint.h>
void mock_gpio_set_pin(uint8_t pin, uint8_t val);
uint8_t mock_gpio_get_pin_mode(uint8_t pin);
uint8_t mock_gpio_get_pin_out(uint8_t pin);
#endif
