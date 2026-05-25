#include "timer_drv.h"
static uint32_t s_tick;

void timer_delay_ms(uint32_t ms) { s_tick += ms; }
void timer_delay_us(uint32_t us) { s_tick += (us / 1000); if (us % 1000) s_tick++; }
uint32_t timer_get_tick(void) { return s_tick; }
void mock_timer_set_tick(uint32_t tick) { s_tick = tick; }
void mock_timer_advance(uint32_t ms) { s_tick += ms; }
