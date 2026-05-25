#ifndef ALARM_SOUND_H
#define ALARM_SOUND_H
#include <stdint.h>

void    alarm_sound_init(void);
uint8_t alarm_sound_load(uint8_t pattern_idx);
void    alarm_sound_tick(uint32_t now);
uint8_t alarm_sound_is_active(void);
void    alarm_sound_stop(void);
uint8_t alarm_sound_current(void);

#endif
