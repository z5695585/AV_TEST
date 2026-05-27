#ifndef ALARM_LIGHT_H
#define ALARM_LIGHT_H
#include <stdint.h>

void     alarm_light_init(void);
uint8_t  alarm_light_load(uint8_t pattern_idx);
void     alarm_light_tick(uint32_t now);
uint8_t  alarm_light_is_active(void);
void     alarm_light_stop(void);
uint8_t  alarm_light_current(void);

void     alarm_light_set_cmd_mode(void);
void     alarm_light_clear_cmd_mode(void);
uint8_t  alarm_light_in_cmd_mode(void);
void     alarm_light_set_freq_ovr(uint32_t hz);
void     alarm_light_set_duty_ovr(uint8_t pct);
void     alarm_light_clear_overrides(void);

#endif
