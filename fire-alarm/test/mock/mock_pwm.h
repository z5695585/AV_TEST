#ifndef MOCK_PWM_H
#define MOCK_PWM_H
#include <stdint.h>
void mock_pwm_reset(void);
uint8_t  mock_pwm_is_running(uint8_t channel);
uint32_t mock_pwm_get_freq(uint8_t channel);
uint8_t  mock_pwm_get_duty(uint8_t channel);
#endif
