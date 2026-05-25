#ifndef MOCK_TIMER_H
#define MOCK_TIMER_H
#include <stdint.h>
void mock_timer_set_tick(uint32_t tick);
void mock_timer_advance(uint32_t ms);
#endif
