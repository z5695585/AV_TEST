/* gpio_drv.c — HC32L110 HAL: GPIO via Gpio_ API
 *
 * Logical pin# = Port×8 + Bit.  Decoded into (port, bit) for SDK API.
 */
#include "gpio_drv.h"
#include "ddl.h"
#include "gpio.h"

void gpio_init(uint8_t pin, uint8_t mode) {
    uint8_t port = pin / 8;
    uint8_t bit  = pin % 8;

    if (mode == GPIO_INPUT) {
        Gpio_InitIOExt(port, bit, GpioDirIn, TRUE, FALSE, FALSE, FALSE);
    } else {
        Gpio_InitIOExt(port, bit, GpioDirOut, FALSE, FALSE, FALSE, FALSE);
    }
}

uint8_t gpio_read(uint8_t pin) {
    uint8_t port = pin / 8;
    uint8_t bit  = pin % 8;
    return (Gpio_GetIO(port, bit) == TRUE) ? 1 : 0;
}

void gpio_write(uint8_t pin, uint8_t level) {
    uint8_t port = pin / 8;
    uint8_t bit  = pin % 8;
    Gpio_SetIO(port, bit, level ? TRUE : FALSE);
}
