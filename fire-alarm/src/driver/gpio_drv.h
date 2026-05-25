#ifndef GPIO_DRV_H
#define GPIO_DRV_H
#include <stdint.h>

#define GPIO_INPUT  0
#define GPIO_OUTPUT 1

void    gpio_init(uint8_t pin, uint8_t mode);
uint8_t gpio_read(uint8_t pin);
void    gpio_write(uint8_t pin, uint8_t level);

#endif
