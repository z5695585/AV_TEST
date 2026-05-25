#ifndef PWM_DRV_H
#define PWM_DRV_H
#include <stdint.h>

void pwm_init(uint8_t channel, uint32_t freq_hz, uint8_t duty_pct);
void pwm_set_freq(uint8_t channel, uint32_t freq_hz);
void pwm_set_duty(uint8_t channel, uint8_t duty_pct);
void pwm_start(uint8_t channel);
void pwm_stop(uint8_t channel);

#endif
