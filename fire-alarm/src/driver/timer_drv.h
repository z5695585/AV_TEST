#ifndef TIMER_DRV_H
#define TIMER_DRV_H
#include <stdint.h>

void     timer_init(void);
void     timer_delay_ms(uint32_t ms);
void     timer_delay_us(uint32_t us);
uint32_t timer_get_tick(void);

#endif
