#ifndef ALARM_SOUND_H
#define ALARM_SOUND_H
#include <stdint.h>

void     alarm_sound_init(void);
uint8_t  alarm_sound_load(uint8_t pattern_idx);
void     alarm_sound_tick(uint32_t now);
uint8_t  alarm_sound_is_active(void);
void     alarm_sound_stop(void);
uint8_t  alarm_sound_current(void);

void     alarm_sound_set_cmd_mode(void);
void     alarm_sound_clear_cmd_mode(void);
uint8_t  alarm_sound_in_cmd_mode(void);
void     alarm_sound_set_freq_ovr(uint32_t hz);
void     alarm_sound_set_duty_ovr(uint8_t pct);
void     alarm_sound_clear_overrides(void);

#endif
