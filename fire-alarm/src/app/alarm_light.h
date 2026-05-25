#ifndef ALARM_LIGHT_H
#define ALARM_LIGHT_H
#include <stdint.h>

void    alarm_light_init(void);
uint8_t alarm_light_load(uint8_t pattern_idx);
void    alarm_light_tick(uint32_t now);
uint8_t alarm_light_is_active(void);
void    alarm_light_stop(void);
uint8_t alarm_light_current(void);

#endif
